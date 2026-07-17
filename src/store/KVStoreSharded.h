#pragma once
#include <optional>
#include <string>
#include <unordered_map>
#include <mutex>
#include <array>

class KVStoreSharded {
private:
    struct alignas(64) Shard {
        std::unordered_map<std::string, std::string> mp;
        std::mutex mtx;
    };

    static constexpr size_t NUM_SHARDS = 16;
    std::array<Shard, NUM_SHARDS> shards_;
    size_t get_shard_index(const std::string& key) const {
        return std::hash<std::string>{}(key) % NUM_SHARDS;
    }

public:
    void set(const std::string& key, const std::string& value) {
        size_t idx = get_shard_index(key);
        std::lock_guard<std::mutex> lock(shards_[idx].mtx);
        shards_[idx].mp[key] = value;
    }

    std::optional<std::string> get(const std::string& key) {
        size_t idx = get_shard_index(key);
        std::lock_guard<std::mutex> lock(shards_[idx].mtx);
        auto it = shards_[idx].mp.find(key);
        if (it == shards_[idx].mp.end()) return std::nullopt;
        return it->second;
    }

    bool del(const std::string& key) {
        size_t idx = get_shard_index(key);
        std::lock_guard<std::mutex> lock(shards_[idx].mtx);
        auto it = shards_[idx].mp.find(key);
        if (it == shards_[idx].mp.end()) return false;
        shards_.at(idx).mp.erase(it);
        return true;
    }
};