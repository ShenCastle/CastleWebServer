#include <list>
#include <vector>
#include <exception>
#include <pthread.h>
#include "../async/async.h"


template<typename T>
class ThreadPool {
private:
    int threadNum;
    int maxReq;
    std::vector<pthread_t> threads;
    std::list<T*> taskQueue;
    Mutex mutex;
    Sem sem;
    static void* handle(void* arg);
    void run();
public:
    ThreadPool(int threadNum_ = 8, int maxReq_ = 10000);
    ~ThreadPool();
    bool addTask(T* req, int state);
};
