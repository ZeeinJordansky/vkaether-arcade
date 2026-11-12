#pragma once
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
namespace vkaether {
class ThreadPool {
public:
 explicit ThreadPool(std::size_t); ~ThreadPool();
 ThreadPool(const ThreadPool&)=delete; ThreadPool& operator=(const ThreadPool&)=delete;
 void submit(std::function<void()>);
private:
 std::mutex mutex_; std::condition_variable ready_; std::queue<std::function<void()>> tasks_;
 std::vector<std::thread> workers_; bool stopping_{false};
};
}