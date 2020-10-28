#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <chrono>
#include <functional>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <assert.h>


typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point Timestamp;
typedef std::function<void()> TimeoutCallback;

struct TimerNode {
    int id;
    Timestamp expires;
    TimeoutCallback cb;
    bool operator<(const TimerNode& t) {
        return expires < t.expires;
    }
};

class HeapTimer {
private:
    void Del_(size_t i);
    void Swap_(size_t i, size_t j);
    bool ShiftDown_(size_t index, size_t n);
    void ShiftUp_(size_t i);
    std::vector<TimerNode> heap_;
    std::unordered_map<int, size_t> ref_;
public:
    HeapTimer();
    ~HeapTimer();
    void Clear();
    void Adjust(int id, int new_expires);
    void Add(int id, int timeout, const TimeoutCallback& cb);
    // void DoWork(int id);
    void Tick();
    void Pop();
    int GetNextTick();
};

#endif
