#pragma once
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

class KVStore {
 private:
  std::unordered_map<std::string, std::string> mp_;

  void cleanUp() { mp_.clear(); }

 public:
  void set(const std::string& key, const std::string& value) {
    mp_[key] = value;
  }

  std::optional<std::string> get(const std::string& key) {
    if (mp_.find(key) == mp_.end()) return std::nullopt;
    return mp_[key];
  }

  bool del(const std::string& key) {
    if (mp_.find(key) == mp_.end()) return false;
    mp_.erase(key);
    return true;
  }
};

class KVStoreMutex {
 private:
  std::unordered_map<std::string, std::string> mp_;
  mutable std::mutex mtx_;

  void cleanUp() { mp_.clear(); }

 public:
  void set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mtx_);
    mp_[key] = value;
  }

  std::optional<std::string> get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = mp_.find(key);
    if (it == mp_.end()) return std::nullopt;
    return it->second;
  }

  bool del(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = mp_.find(key);
    if (it == mp_.end()) return false;
    mp_.erase(it);
    return true;
  }
};

class KVStoreSharedMutex {
 private:
  std::unordered_map<std::string, std::string> mp_;
  mutable std::shared_mutex mtx_;

 public:
  void set(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(
        mtx_);  // Exclusive lock for writing
    mp_[key] = value;
  }

  std::optional<std::string> get(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(mtx_);  // shared lock for reading
    auto it = mp_.find(key);
    if (it == mp_.end()) return std::nullopt;
    return it->second;
  }

  bool del(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(
        mtx_);  // Exclusive lock for writing
    auto it = mp_.find(key);
    if (it == mp_.end()) return false;
    mp_.erase(it);
    return true;
  }
};