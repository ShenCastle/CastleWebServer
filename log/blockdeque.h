#include <deque>
#include <mutex>
#include <condition_variable>
#include <chrono>


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
