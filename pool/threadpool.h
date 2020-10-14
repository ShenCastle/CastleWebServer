#include <list>
#include <exception>
#include <pthread.h>
#include "../async/async.h"


template<typename T>
class threadpool {
private:
    int threadNum;
    int maxReq;
    pthread_t* threads;
    std::list<T*> workQueue;
    Mutex mutex;
    Sem sem;
    static void* worker(void* arg);
    void run();
public:
    threadpool(int threadNum_ = 8, int maxReq_ = 10000);
    ~threadpool();
    bool append(T* req, int state);
    bool append_p(T* req);
};
