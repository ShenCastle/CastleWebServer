#include "threadpool.h"


template<typename T>
ThreadPool<T>::ThreadPool(int threadNum_, int maxReq_): threadNum(threadNum_), maxReq(maxReq_) {
    if (threadNum_ <= 0 || maxReq_ <= 0) {
        throw std::exception();
    }
    threads.resize(threadNum);
    for (int i = 0; i < threadNum; i++) {
        if (pthread_create(threads, nullptr, handle, this)) {
            throw std::exception();
        }
        if (pthread_detach(threads[i])) {
            throw std::exception();
        }
    }
}
template<typename T>
ThreadPool<T>::~ThreadPool() {}
template<typename T>
bool ThreadPool<T>::addTask(T* req, int state) {
    mutex.lock();
    if (taskQueue.size() >= maxReq) {
        mutex.unlock();
        return false;
    }
    req->state = state;
    taskQueue.push_back(req);
    mutex.unlock();
    sem.post();
    return true;
}
template<typename T>
void* ThreadPool<T>::handle(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;
    pool->run();
    return pool;
}
template<typename T>
void ThreadPool<T>::run() {
    while (true) {
        sem.wait();
        mutex.lock();
        if (taskQueue.empty()) {
            mutex.unlock();
            continue;
        }
        T* req = taskQueue.front();
        taskQueue.pop_front();
        mutex.unlock();
        if (!req) {
            continue;
        }
        if (req->state == 0) {
            if (req->readOnce()) {
                req->improv = 1;
                req.process();
            }
            else {
                req->improv = 1;
                req->timerFlag = 1;
            }
        }
        else {
            if (req->write()) {
                req->improv = 1;
            }
            else {
                req->improv = 1;
                req->timerFlag = 1;
            }
        }
    }
}
