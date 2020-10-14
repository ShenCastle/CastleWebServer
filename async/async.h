#include <exception>
#include <pthread.h>
#include <semaphore.h>


class Sem {
private:
    sem_t sem_;
public:
    Sem();
    Sem(int num);
    ~Sem();
    bool wait();
    bool post();
};

class Mutex {
private:
    pthread_mutex_t mutex_;
public:
    Mutex();
    ~Mutex();
    bool lock();
    bool unlock();
    pthread_mutex_t* get();
};

class Cond {
private:
    pthread_cond_t cond_;
public:
    Cond();
    ~Cond();
    bool wait(pthread_mutex_t* mutex_);
    bool timedwait(pthread_mutex_t* mutex_, timespec t);
    bool broadcast();
    bool signal();
};

