#pragma once
#include <fcntl.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <array>

#include "net/RespParser.h"
#include "store/KVStoreSharded.h"

struct WorkerContext {
  int epfd;               // this thread's epoll instance
  int pipe_read_fd;       // read end of its pipe
  KVStoreSharded& store;  // shared reference
  RespParser parser;      // one per thread, no sharing needed
  WorkerContext(int epoll_fd, int read_fd, KVStoreSharded& obj) : epfd(epoll_fd),
    pipe_read_fd(read_fd), store(obj) {}
};

class Server {
 private:
  KVStoreSharded& store_;
  RespParser parser_;
  std::string port_;  // the port users will be connecting to
  int backlog_{10};   // how many pending connections queue holds
  int num_threads_{std::thread::hardware_concurrency()};

  void worker_loop(WorkerContext ctx) {
    std::cout << "thread spawned: " << std::this_thread::get_id() << "\n";
    struct epoll_event ev;
    struct epoll_event events[64];
    char buf[1024];
    while (true) {
      int n = epoll_wait(ctx.epfd, events, 64, -1);
      for (int i = 0; i < n; i++) {
        if (events[i].data.fd == ctx.pipe_read_fd) { // new connection
          int new_fd;
          read(ctx.pipe_read_fd, &new_fd, sizeof(new_fd));
          fcntl(new_fd, F_SETFL, O_NONBLOCK);
          ev.events = EPOLLIN;
          ev.data.fd = new_fd;
          epoll_ctl(ctx.epfd, EPOLL_CTL_ADD, new_fd, &ev);
        } else {
          int client_fd = events[i].data.fd;
          int bytes = recv(client_fd, buf, sizeof(buf)-1, 0);
          if (bytes <= 0) {
            epoll_ctl(ctx.epfd, EPOLL_CTL_DEL, client_fd, nullptr);
            close(client_fd);
            continue;
          }
          buf[bytes] = '\0';
          auto tokens = ctx.parser.parse(buf);
          if (tokens.empty()) continue;
          int bytes_sent;
          if (tokens[0] == "SET") {
            if (tokens.size() != 3) {
              continue;
            }
            ctx.store.set(tokens[1], tokens[2]);
            std::string msg = "+OK\r\n";
            bytes_sent = send(client_fd, msg.c_str(), msg.size(), 0);
            if (bytes_sent == -1) {
              std::cout << "ERROR while sending status on SET command\n";
            }
          } else if (tokens[0] == "GET") {
            if (tokens.size() != 2) {
              continue;
            }
            if (auto result = ctx.store.get(tokens[1])) {
              // Safe to access: result contains a value
              std::string msg = ("$" + std::to_string(result->size()) + "\r\n" +
                                 *result + "\r\n");
              bytes_sent = send(client_fd, msg.c_str(), msg.size(), 0);
            } else {
              std::string msg = "$-1\r\n";
              bytes_sent = send(client_fd, msg.c_str(), msg.size(), 0);
            }
            if (bytes_sent == -1) {
              std::cout << "ERROR while sending status on GET command\n";
            }
          } else if (tokens[0] == "DEL") {
            if (tokens.size() != 2) {
              continue;
            }
            if (ctx.store.del(tokens[1])) {
              std::string msg = ":1\r\n";
              bytes_sent = send(client_fd, msg.c_str(), msg.size(), 0);
            } else {
              std::string msg = ":0\r\n";
              bytes_sent = send(client_fd, msg.c_str(), msg.size(), 0);
            }
            if (bytes_sent == -1) {
              std::cout << "ERROR while sending status on GET command\n";
            }
          } else if (tokens[0] == "EXPIRE") {   
            if (tokens.size() != 3) {
              continue;
            }
            if (ctx.store.expire(tokens[1], std::stoi(tokens[2]))) {
              std::string msg = ":1\r\n";
              bytes_sent = send(client_fd, msg.c_str(), msg.size(), 0);
            } else {
              std::string msg = ":0\r\n";
              bytes_sent = send(client_fd, msg.c_str(), msg.size(), 0);
            }
            if (bytes_sent == -1) {
              std::cout << "ERROR while sending status on EXPIRE command\n";
            }
          } else {
            std::string msg = "-ERR unknown command\r\n";
            bytes_sent = send(client_fd, msg.c_str(), msg.size(), 0);
            if (bytes_sent == -1) {
              std::cout << "ERROR while sending status on receiving unknown "
                           "command\n";
            }
          }
        }
      }
    }
  }

 public:
  Server(const std::string& port, KVStoreSharded& store)
      : store_(store), port_(port) {}

  void run() {
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints, *res;
    int sockfd, new_fd;

    // first, load up address structs with getaddrinfo():
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;  // fill in my IP for me

    getaddrinfo(NULL, port_.c_str(), &hints, &res);

    // make a socket, bind it, and listen on it:

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) std::cout << "ERROR: Socket connection failed!\n";

    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    int check = bind(sockfd, res->ai_addr, res->ai_addrlen);
    if (check == -1) std::cout << "ERROR: call to bind failed!\n";

    check = listen(sockfd, backlog_);
    if (check == -1) std::cout << "ERROR: Call to listen failed!\n";

    struct epoll_event ev;
    std::vector<std::array<int,2>> pipes(num_threads_);
    std::vector<int> epfds(num_threads_);
    std::vector<WorkerContext> contexts;
    for (int i = 0; i < num_threads_; i++) {
      pipe(pipes[i].data());
      epfds[i] = epoll_create1(0);
      ev.events = EPOLLIN;
      ev.data.fd = pipes[i][0];
      epoll_ctl(epfds[i], EPOLL_CTL_ADD, pipes[i][0], &ev);
      contexts.push_back(WorkerContext{epfds[i], pipes[i][0], store_});
    }
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads_; i++) {
      threads.emplace_back(&Server::worker_loop, this, contexts[i]);
    }

    // now accept an incoming connection:
    addr_size = sizeof their_addr;
    int idx = 0;
    while (true) {
      // new connection — accept(), make non-blocking, register with epoll
      new_fd = accept(sockfd, (struct sockaddr*)&their_addr, &addr_size);
      if (new_fd == -1) {
        std::cout << "ERROR in accept()ing...\n";
        continue;
      }
      write(pipes[idx % num_threads_][1], &new_fd, sizeof(new_fd));
      idx++;
    }
  }
};