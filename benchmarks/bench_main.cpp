#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include <random>
#include <functional>
#include <algorithm>
#include <cmath>

#include "KVStore.h"

template <typename T>
void worker(T& store, long ops, int read_ratio, std::vector<long>& latencies) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distrib(1, 10000);
    std::uniform_int_distribution<int> op_distrib(0, 99);
    while(ops--) {
        int random_num = distrib(gen);
        std::string key = "key_" + std::to_string(random_num);
        auto start = std::chrono::steady_clock::now();
        if(op_distrib(gen) < read_ratio) {
            store.get(key);
        }
        else {
            store.set(key, "val_" + std::to_string(ops) + "_" + std::to_string(random_num));
        }
        auto end = std::chrono::steady_clock::now();
        auto elapsed_mus = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        latencies.push_back(elapsed_mus.count());
    }
}

int main(int argc, char *argv[]) {
    int num_threads = 4;
    int read_ratio  = 95;   // % of ops that are GETs
    long total_ops  = 1000000;
    std::string store_type = "mutex"; 
    for(int i = 1; i < argc; i++) {
        if((std::string(argv[i]) == "--threads") && ((i+1) < argc)) {
            num_threads = std::stoi(argv[i+1]);
        }
        else if((std::string(argv[i]) == "--ratio") && ((i+1) < argc)) {
            read_ratio = std::stoi(argv[i+1]);
        }
        else if((std::string(argv[i]) == "--ops") && ((i+1) < argc)) {
            total_ops = std::stol(argv[i+1]);
        }
        else if((std::string(argv[i]) == "--store") && ((i+1) < argc)) {
            store_type = argv[i+1];
        }
    }
    std::cout << "Thread_cnt=" << num_threads << ", read ratio=" << read_ratio
                << ", total operations=" << total_ops << ", store_type=" << store_type <<"\n";

    KVStoreMutex store_obj;
    KVStoreSharedMutex store_obj_shared;

    if(store_type == "mutex") {
        for(int i = 1; i <= 10000; i++) {
            store_obj.set("key_" + std::to_string(i), "value_" + std::to_string(i));
        }
    }
    else {
        for(int i = 1; i <= 10000; i++) {
            store_obj_shared.set("key_" + std::to_string(i), "value_" + std::to_string(i));
        }
    }

    long ops_per_thread = total_ops / num_threads;
    std::vector<std::vector<long>> lat_vec(num_threads); // latency vector

    auto start = std::chrono::steady_clock::now();
    std::vector<std::thread> threads;
    for(int i = 0; i < num_threads; i++) {
        if(store_type == "mutex") {
            threads.emplace_back(worker<KVStoreMutex>, std::ref(store_obj), ops_per_thread, read_ratio, std::ref(lat_vec[i]));
        }
        else {
            threads.emplace_back(worker<KVStoreSharedMutex>, std::ref(store_obj_shared), ops_per_thread, read_ratio, std::ref(lat_vec[i]));
        }
    }
    
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    auto end = std::chrono::steady_clock::now();
    double elapsed_sec = std::chrono::duration<double>(end - start).count();
    double ops_per_sec = total_ops / elapsed_sec;

    std::vector<long> flat_lat_vec;
    size_t total_size = 0;
    for (const auto& row : lat_vec) {
        total_size += row.size();
    }
    flat_lat_vec.reserve(total_size);
    for (const auto& row : lat_vec) {
        flat_lat_vec.insert(flat_lat_vec.end(), row.begin(), row.end());
    }

    sort(flat_lat_vec.begin(), flat_lat_vec.end());
    size_t p99_index = static_cast<size_t>(std::ceil(0.99 * flat_lat_vec.size())) - 1;
    std::cout << "ops/sec: " << ops_per_sec << "\n";
    std::cout << "p99 latency: " << flat_lat_vec[p99_index] << "\n";
}