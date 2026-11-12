#include "vkaether/thread_pool.hpp"
#include <algorithm>
#include <stdexcept>
namespace vkaether {
ThreadPool::ThreadPool(std::size_t n){if(!n)n=std::max(2u,std::thread::hardware_concurrency());workers_.reserve(n);for(std::size_t i=0;i<n;++i)workers_.emplace_back([this]{for(;;){std::function<void()> task;{std::unique_lock lock(mutex_);ready_.wait(lock,[this]{return stopping_||!tasks_.empty();});if(stopping_&&tasks_.empty())return;task=std::move(tasks_.front());tasks_.pop();}try{task();}catch(...){}}});}
ThreadPool::~ThreadPool(){{std::lock_guard lock(mutex_);stopping_=true;}ready_.notify_all();for(auto& w:workers_)if(w.joinable())w.join();}
void ThreadPool::submit(std::function<void()> task){{std::lock_guard lock(mutex_);if(stopping_)throw std::runtime_error("pool stopping");tasks_.push(std::move(task));}ready_.notify_one();}
}