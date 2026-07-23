#include <unistd.h>

#include <cstring>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "net/RespParser.h"
#include "net/Server.h"
#include "store/KVStoreSharded.h"

int main() {
  KVStoreSharded store;
  std::string port = "6380";
  Server server(port, store);
  server.run();
}
