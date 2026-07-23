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
#include <unordered_map>
#include <vector>

#include "net/RespParser.h"
#include "store/KVStoreSharded.h"

class Server {
 private:
  KVStoreSharded& store_;
  RespParser parser_;
  std::string port_;  // the port users will be connecting to
  int backlog_{10};   // how many pending connections queue holds

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

    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    int epfd = epoll_create1(0);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

    // now accept an incoming connection:

    addr_size = sizeof their_addr;
    char buf[1024];
    struct epoll_event events[64];
    while (true) {
      int n = epoll_wait(epfd, events, 64, -1);
      for (int i = 0; i < n; i++) {
        if (events[i].data.fd == sockfd) {
          // new connection — accept(), make non-blocking, register with epoll
          new_fd = accept(sockfd, (struct sockaddr*)&their_addr, &addr_size);
          if (new_fd == -1) {
            std::cout << "ERROR in accept()ing...\n";
            continue;
          }
          fcntl(new_fd, F_SETFL, O_NONBLOCK);
          ev.events = EPOLLIN;
          ev.data.fd = new_fd;
          epoll_ctl(epfd, EPOLL_CTL_ADD, new_fd, &ev);
        } else {
          // existing connection — recv(), parse, respond
          // if bytes <= 0: epoll_ctl remove + close
          // inner loop: keep reading from this client until it disconnects
          int client_fd = events[i].data.fd;
          int bytes = recv(client_fd, buf, sizeof(buf) - 1, 0);
          if (bytes <= 0) {
            epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, nullptr);
            close(client_fd);
            continue;
          }
          buf[bytes] = '\0';

          std::vector<std::string> tokens = parser_.parse(buf);
          if (tokens.empty()) continue;
          int bytes_sent;
          if (tokens[0] == "SET") {
            if (tokens.size() != 3) {
              continue;
            }
            store_.set(tokens[1], tokens[2]);
            std::string msg = "+OK\r\n";
            bytes_sent = send(client_fd, msg.c_str(), msg.size(), 0);
            if (bytes_sent == -1) {
              std::cout << "ERROR while sending status on SET command\n";
            }
          } else if (tokens[0] == "GET") {
            if (tokens.size() != 2) {
              continue;
            }
            if (auto result = store_.get(tokens[1])) {
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
            if (store_.del(tokens[1])) {
              std::string msg = ":1\r\n";
              bytes_sent = send(client_fd, msg.c_str(), msg.size(), 0);
            } else {
              std::string msg = ":0\r\n";
              bytes_sent = send(client_fd, msg.c_str(), msg.size(), 0);
            }
            if (bytes_sent == -1) {
              std::cout << "ERROR while sending status on GET command\n";
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
};