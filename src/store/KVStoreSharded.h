#pragma once
#include <array>
#include <chrono>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unistd.h>
#include <unordered_map>

struct Entry {
  std::string value;
  std::optional<std::chrono::steady_clock::time_point> expires_at;
};

class KVStoreSharded {
 private:
  struct alignas(64) Shard {
    std::unordered_map<std::string, Entry> mp;
    std::mutex mtx;
    std::list<std::string> lru_list;
    std::unordered_map<std::string, std::list<std::string>::iterator> lru_map;
  };

  static constexpr size_t NUM_SHARDS = 16;
  static constexpr size_t MAX_KEYS_PER_SHARD = 1000;
  std::array<Shard, NUM_SHARDS> shards_;
  size_t get_shard_index(const std::string& key) const {
    return std::hash<std::string>{}(key) % NUM_SHARDS;
  }

  std::jthread reaper_;

 public:
  KVStoreSharded() {
    reaper_ = std::jthread([this](std::stop_token st) {
      while (!st.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        purge_expired();
      }
    });
  }

  void set(const std::string& key, const std::string& value) {
    size_t idx = get_shard_index(key);
    std::lock_guard<std::mutex> lock(shards_[idx].mtx);
    Entry entry = {value, std::nullopt};
    if (!shards_[idx].mp.contains(key) &&
        shards_[idx].mp.size() >= MAX_KEYS_PER_SHARD) {
      std::string& del_key = shards_[idx].lru_list.back();
      shards_[idx].mp.erase(del_key);
      shards_[idx].lru_map.erase(del_key);
      shards_[idx].lru_list.pop_back();
    }
    shards_[idx].mp[key] = entry;
    if (shards_[idx].lru_map.contains(key)) {
      auto it = shards_[idx].lru_map[key];
      shards_[idx].lru_list.erase(it);
    }
    shards_[idx].lru_list.emplace_front(key);
    shards_[idx].lru_map[key] = shards_[idx].lru_list.begin();
  }

  std::optional<std::string> get(const std::string& key) {
    size_t idx = get_shard_index(key);
    std::lock_guard<std::mutex> lock(shards_[idx].mtx);
    auto it = shards_[idx].mp.find(key);
    if (it == shards_[idx].mp.end()) return std::nullopt;
    if (it->second.expires_at.has_value() &&
        it->second.expires_at.value() <
            std::chrono::steady_clock::now()) {  // key expired
      shards_[idx].mp.erase(it);
      shards_[idx].lru_list.erase(shards_[idx].lru_map[key]);
      shards_[idx].lru_map.erase(key);
      return std::nullopt;
    }
    if (shards_[idx].lru_map.contains(key)) {
      auto it = shards_[idx].lru_map[key];
      shards_[idx].lru_list.erase(it);
    }
    shards_[idx].lru_list.emplace_front(key);
    shards_[idx].lru_map[key] = shards_[idx].lru_list.begin();
    return (it->second).value;
  }

  bool del(const std::string& key) {
    size_t idx = get_shard_index(key);
    std::lock_guard<std::mutex> lock(shards_[idx].mtx);
    auto it = shards_[idx].mp.find(key);
    if (it == shards_[idx].mp.end()) return false;
    shards_.at(idx).mp.erase(it);
    shards_[idx].lru_list.erase(shards_[idx].lru_map[key]);
    shards_[idx].lru_map.erase(key);
    return true;
  }

  bool expire(const std::string& key, int seconds) {
    size_t idx = get_shard_index(key);
    std::lock_guard<std::mutex> lock(shards_[idx].mtx);
    auto it = shards_[idx].mp.find(key);
    if (it == shards_[idx].mp.end()) return false;
    shards_[idx].mp[key].expires_at =
        std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    return true;
  }

  size_t size() {
    size_t ans = 0;
    for(size_t i = 0; i < NUM_SHARDS; i++) {
      auto& shard = shards_[i];
      std::lock_guard<std::mutex> lock(shard.mtx);
      ans += shard.mp.size();
    }
    return ans;
  }

  void purge_expired() {
    const auto now = std::chrono::steady_clock::now();
    for (size_t i = 0; i < NUM_SHARDS; i++) {
      auto& shard = shards_[i];
      std::lock_guard<std::mutex> lock(shard.mtx);
      for (auto it = shard.mp.begin(); it != shard.mp.end();
           /* updated inside */) {
        if (it->second.expires_at.has_value() &&
            it->second.expires_at.value() < now) {
          // Safe erase: erase() returns the valid iterator to the NEXT element
          std::string key = it->first;
          it = shard.mp.erase(it);
          shard.lru_list.erase(shard.lru_map[key]);
          shard.lru_map.erase(key);
        } else {
          ++it;
        }
      }
    }
  }
};