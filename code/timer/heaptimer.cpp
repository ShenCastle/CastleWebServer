#include "heaptimer.h"


HeapTimer::HeapTimer() {
    heap_.reserve(64);
}

HeapTimer::~HeapTimer() {
    Clear();
}

void HeapTimer::Clear() {
    heap_.clear();
    ref_.clear();
}

void HeapTimer::Adjust(int id, int timeout) {
    assert(!heap_.empty() && ref_.count(id));
    heap_[ref_[id]].expires = Clock::now() + MS(timeout);
    ShiftDown_(ref_[id], heap_.size());
}

void HeapTimer::Add(int id, int timeout, const TimeoutCallback& cb) {
    assert(id >= 0);
    size_t i;
    if (ref_.count(id)) {
        i = ref_[id];
        heap_[i].expires = Clock::now() + MS(timeout);
        heap_[i].cb = cb;
        if (!ShiftDown_(i, heap_.size())) {
            ShiftUp_(i);
        }
    }
    else {
        i = heap_.size();
        ref_[id] = i;
        heap_.push_back({id, Clock::now() + MS(timeout), cb});
        ShiftUp_(i);
    }
}

void HeapTimer::Tick() {
    if (heap_.empty()) {
        return;
    }
    while (!heap_.empty()) {
        TimerNode node = heap_.front();
        if (std::chrono::duration_cast<MS>(node.expires - Clock::now()).count()) {
            break;
        }
        node.cb();
        Pop();
    }
}

void HeapTimer::Pop() {
    assert(!heap_.empty());
    Del_(0);
}

int HeapTimer::GetNextTick() {
    Tick();
    size_t res = -1;
    if (!heap_.empty()) {
        res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
        res = res < 0 ? 0 : res;
    }
    return res;
}

void HeapTimer::Del_(size_t i_) {
    int i = i_;
    int n = heap_.size() - 1;
    assert(!heap_.empty() && i >= 0 && i <= n);
    if (i < n) {
        Swap_(i, n);
        if (!ShiftDown_(i, n)) {
            ShiftUp_(i);
        }
    }
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

void HeapTimer::Swap_(size_t i, size_t j) {
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}

bool HeapTimer::ShiftDown_(size_t index, size_t n) {
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());
    size_t i = index;
    size_t j = i * 2 + 1;
    while (j < n) {
        if (j + 1 < n && heap_[j + 1] < heap_[j]) {
            j++;
        }
        if (heap_[i] < heap_[j]) {
            break;
        }
        Swap_(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

void HeapTimer::ShiftUp_(size_t i) {
    assert(i >= 0 && i < heap_.size());
    size_t j = (i - 1) / 2;
    while (j >= 0) {
        if (heap_[j] < heap_[i]) {
            break;
        }
        Swap_(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}
