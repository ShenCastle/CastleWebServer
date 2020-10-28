#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <thread>
#include <assert.h>


class ThreadPool
{
private:
    struct Pool {
        std::mutex mtx;
        std::condition_variable cond;
        bool is_close;
        std::queue<std::function<void()>> tasks;
    };
    std::shared_ptr<Pool> pool_;
public:
    explicit ThreadPool(size_t thread_num = 8);
    ThreadPool() = default;
    ThreadPool(ThreadPool&&) = default;
    ~ThreadPool();
    template<typename T>
    void AddTask(T&& task) {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->tasks.emplace(std::forward<T>(task));
        }
        pool_->cond.notify_one();
    }
};

#endif
