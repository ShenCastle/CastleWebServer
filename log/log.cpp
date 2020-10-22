#include "log.h"


Log::Log() {
    line_cnt_ = 0;
    to_day_ = 0;
    is_async_ = false;
    fp_ = nullptr;
    deque_ = nullptr;
    write_thread_ = nullptr;    
}
