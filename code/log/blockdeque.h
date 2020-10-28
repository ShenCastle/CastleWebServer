#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <deque>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <assert.h>


template<typename T>
class BlockDeque {
private:
    std::deque<T> deq_;
    size_t capacity_;
    std::mutex mutex_;
    std::condition_variable cond_producer;
    std::condition_variable cond_consumer_;
    bool is_close_;
public:
    explicit BlockDeque(size_t max_capacity = 1000);
    ~BlockDeque();
    void Close();
    void PushBack(const T& item);
    void PushFront(const T& item);
    bool Pop(T& item);
    bool Pop(T& item, int timeout);
    void Flush();
    void Clear();
    bool IsEmpty();
    bool IsFull();
    size_t GetSize();
    size_t GetCapacity();
    T GetFront();
    T GetBack();
};

template<typename T>
BlockDeque<T>::BlockDeque(size_t max_capacity) :capacity_(max_capacity) {
    assert(max_capacity > 0);
    is_close_ = false;
}

template<typename T>
BlockDeque<T>::~BlockDeque() {
    Close();
};

template<typename T>
void BlockDeque<T>::Close() {
    {
        std::lock_guard<std::mutex> locker(mutex_);
        deq_.clear();
        is_close_ = true;
    }
    cond_producer.notify_all();
    cond_consumer_.notify_all();
}

template<typename T>
void BlockDeque<T>::PushFront(const T& item) {
    std::unique_lock<std::mutex> locker(mutex_);
    while (deq_.size() >= capacity_) {
        cond_producer.wait(locker);
    }
    deq_.push_front(item);
    cond_consumer_.notify_one();
}

template<typename T>
void BlockDeque<T>::PushBack(const T& item) {
    std::unique_lock<std::mutex> locker(mutex_);
    while (deq_.size() >= capacity_) {
        cond_producer.wait(locker);
    }
    deq_.push_back(item);
    cond_consumer_.notify_one();
}

template<typename T>
bool BlockDeque<T>::Pop(T& item) {
    std::unique_lock<std::mutex> locker(mutex_);
    while (deq_.empty()) {
        cond_consumer_.wait(locker);
        if (is_close_) {
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    cond_producer.notify_one();
    return true;
}

template<typename T>
bool BlockDeque<T>::Pop(T& item, int timeout) {
    std::unique_lock<std::mutex> locker(mutex_);
    while (deq_.empty()) {
        if (cond_consumer_.wait_for(locker, std::chrono::seconds(timeout))
            == std::cv_status::timeout) {
            return false;
        }
        if (is_close_) {
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    cond_producer.notify_one();
    return true;
}

template<typename T>
void BlockDeque<T>::Flush() {
    cond_consumer_.notify_one();
};

template<typename T>
void BlockDeque<T>::Clear() {
    std::lock_guard<std::mutex> locker(mutex_);
    deq_.clear();
}

template<typename T>
bool BlockDeque<T>::IsEmpty() {
    std::lock_guard<std::mutex> locker(mutex_);
    return deq_.empty();
}

template<typename T>
bool BlockDeque<T>::IsFull() {
    std::lock_guard<std::mutex> locker(mutex_);
    return deq_.size() >= capacity_;
}

template<typename T>
size_t BlockDeque<T>::GetSize() {
    std::lock_guard<std::mutex> locker(mutex_);
    return deq_.size();
}

template<typename T>
size_t BlockDeque<T>::GetCapacity() {
    std::lock_guard<std::mutex> locker(mutex_);
    return capacity_;
}

template<typename T>
T BlockDeque<T>::GetFront() {
    std::lock_guard<std::mutex> locker(mutex_);
    return deq_.front();
}

template<typename T>
T BlockDeque<T>::GetBack() {
    std::lock_guard<std::mutex> locker(mutex_);
    return deq_.back();
}

#endif
