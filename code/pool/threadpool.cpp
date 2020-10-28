#include "threadpool.h"


ThreadPool::ThreadPool(size_t thead_num): pool_(std::make_shared<Pool>()) {
    assert(thead_num > 0);
    for (size_t i = 0; i < thead_num; i++) {
        std::thread([pool = pool_] {
            std::unique_lock<std::mutex> locker(pool->mtx);
            while (true) {
                if (!pool->tasks.empty()) {
                    auto task = std::move(pool->tasks.front());
                    pool->tasks.pop();
                    locker.unlock();
                    task();
                    locker.lock();
                }
                else if (pool->is_close) {
                    break;
                }
                else {
                    pool->cond.wait(locker);
                }
            }
        }).detach();
    }
}

ThreadPool::~ThreadPool() {
    if (static_cast<bool>(pool_)) {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->is_close = true;
        }
        pool_->cond.notify_all();
    }
}

template<typename T>
void ThreadPool::AddTask(T&& task) {
    {
        std::lock_guard<std::mutex> locker(pool_->mtx);
        pool_->tasks.emplace(std::forward<T>(task));
    }
    pool_->cond.notify_one();
}
