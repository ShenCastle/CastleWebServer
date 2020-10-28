#include "sync.h"


Sem::Sem() {
    if (sem_init(&sem, 0, 0) != 0) {
        throw std::exception();
    }
}
Sem::Sem(int num) {
    if (sem_init(&sem, 0, num) != 0) {
        throw std::exception();
    }
}
Sem::~Sem() {
    sem_destroy(&sem);
}
bool Sem::wait() {
    return sem_wait(&sem) == 0;
}
bool Sem::post() {
    return sem_post(&sem) == 0;
}

Mutex::Mutex() {
    if (pthread_mutex_init(&mutex, nullptr) != 0) {
        throw std::exception();
    }
}
Mutex::~Mutex() {
    pthread_mutex_destroy(&mutex);
}
bool Mutex::lock() {
    return pthread_mutex_lock(&mutex) == 0;
}
bool Mutex::unlock() {
    return pthread_mutex_unlock(&mutex) == 0;
}
pthread_mutex_t* Mutex::get() {
    return &mutex;
}

Cond::Cond() {
    if (pthread_cond_init(&cond, nullptr) != 0) {
        throw std::exception();
    }
}
Cond::~Cond() {
    pthread_cond_destroy(&cond);
}
bool Cond::wait(pthread_mutex_t* mutex_) {
    return pthread_cond_wait(&cond, mutex_) == 0;
}
bool Cond::timedwait(pthread_mutex_t* mutex_, timespec t) {
    return pthread_cond_timedwait(&cond, mutex_, &t) == 0;
}
bool Cond::signal() {
    return pthread_cond_signal(&cond) == 0;
}
bool Cond::broadcast() {
    return pthread_cond_broadcast(&cond) == 0;
}
