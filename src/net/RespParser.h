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

class RespParser {
 public:
  std::vector<std::string> parse(const std::string& raw) {
    std::vector<std::string> tokens;
    std::vector<std::string> parsed;
    std::string delimiter = "\r\n";
    size_t start = 0;
    size_t end = raw.find(delimiter);

    // Loop while the delimiter is found in the remaining string
    while (end != std::string::npos) {
      tokens.push_back(raw.substr(start, end - start));
      start = end + delimiter.length();  // Move past the delimiter
      end = raw.find(delimiter, start);
    }

    // Push the remaining part of the string
    tokens.push_back(raw.substr(start));
    if (tokens[0][0] == '*') {
      int num_args = std::stoi(tokens[0].substr(1));
      int i = 1;
      for (int j = 0; j < num_args; j++) {
        i++;  // skip the $N line
        parsed.push_back(tokens[i]);
        i++;
      }
    }
    return parsed;
  }
};