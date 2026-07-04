#pragma once
#include <unordered_map>
#include <string>
#include <optional>

class KVStore {
private:
    std::unordered_map<std::string, std::string> mp_;

    void cleanUp() {
        mp_.clear();
    }

public:
    void set(const std::string& key, const std::string& value) {
        mp_[key] = value;
    }

    std::optional<std::string> get(const std::string& key) {
        if(mp_.find(key) == mp_.end()) return std::nullopt;
        return mp_[key];
    }

    bool del(const std::string& key) {
        if(mp_.find(key) == mp_.end()) return false;
        mp_.erase(key);
        return true;
    }
};