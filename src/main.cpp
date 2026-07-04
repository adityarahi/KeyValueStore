#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <optional>
#include <sstream>
#include "KVStore.h"

int main() {
    KVStore obj;
    while(true) {
        std::string input;
        std::getline(std::cin, input);
        std::stringstream ss(input);
        std::string token;
        std::vector<std::string> tokens;

        while(ss >> token) {
            tokens.push_back(token);
        }

        if (tokens.empty()) {
            std::cout << "No command entered." << std::endl;
        }
        else if(tokens[0] == "EXIT") {
            std::cout << "Exiting...\n";
            return 0;
        }
        else if(tokens[0] == "GET") {
            if(tokens.size() != 2) {
                std::cout << "invalid command: usage GET <keyname>\n";
            }
            else {
                auto maybe_value = obj.get(tokens[1]);
                if(maybe_value.has_value()) {
                    std::cout << maybe_value.value() << "\n";
                }
                else {
                    std::cout << "Entered key is not stored\n";
                }
            }
        }
        else if(tokens[0] == "SET") {
            if(tokens.size() != 3) {
                std::cout << "invalid command: usage SET <keyname> <valuename>\n";
            }
            else {
                obj.set(tokens[1], tokens[2]);
                std::cout << "OK\n";
            }
        }
        else if(tokens[0] == "DEL") {
            if(tokens.size() != 2) {
                std::cout << "invalid command: usage DEL <keyname>\n";
            }
            else {
                if(obj.del(tokens[1])) {
                    std::cout << tokens[1] << " : key deleted\n";
                }
                else std::cout << "key does not exist!\n";
            }
            
        }
        else {
            std::cout << "Invalid command, supported commands are GET, SET, DEL, EXIT\n";
        }
    }
}