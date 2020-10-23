#include "log.h"
using namespace std;


Log::Log() {
    line_cnt_ = 0;
    to_day_ = 0;
    is_async_ = false;
    fp_ = nullptr;
    deque_ = nullptr;
    write_thread_ = nullptr;    
}

Log::~Log() {
    if (write_thread_ && write_thread_->joinable()) {
        while (!deque_->IsEmpty()) {
            deque_->Flush();
        }
        deque_->Close();
        write_thread_->join();
    }
    if (fp_) {
        lock_guard<mutex> locker(mtx_);
        Flush();
        fclose(fp_);
    }
}

void Log::Init(int level, const char* path, const char* suffix, int max_queue_capacity) {
    is_open_ = true;
    level_ = level;
    if (max_queue_capacity > 0) {
        is_async_ = true;
        if (!deque_) {
            unique_ptr<BlockDeque<string>> new_deque(new BlockDeque<string>);
            deque_ = move(new_deque);
            unique_ptr<thread> new_thread(new thread(FlushLogThread));
            write_thread_ = move(new_thread);
        }
    }
    else {
        is_async_ = false;
    }
    line_cnt_ = 0;
    time_t timer = time(nullptr);
    struct tm* sys_time = localtime(&timer);
    struct tm t = *sys_time;
    path_ = path;
    suffix_ = suffix;
    char file_name[LOG_NAME_LEN] = {0};
    snprintf(file_name, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s",
            path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
    to_day_ = t.tm_mday;
    {
        lock_guard<mutex> locker(mtx_);
        buff_.RetrieveAll();
        if (fp_) {
            Flush();
            fclose(fp_);
        }
        fp_ = fopen(file_name, "a");
        if (!fp_) {
            mkdir(path_, 0777);
            fp_ = fopen(file_name, "a");
        }
        assert(fp_ != nullptr);
    }
}

void Log::Write(int level, const char* format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t t_sec = now.tv_sec;
    struct tm *sys_time = localtime(&t_sec);
    struct tm t = *sys_time;
    va_list vl;
    if (to_day_ != t.tm_mday || (line_cnt_ && (line_cnt_ % MAX_LINES == 0))) {
        unique_lock<mutex> locker(mtx_);
        locker.unlock();
        char file_name[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
        if (to_day_ != t.tm_mday) {
            snprintf(file_name, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
            to_day_ = t.tm_mday;
            line_cnt_ = 0;
        }
        else {
            snprintf(file_name, LOG_NAME_LEN - 72, "%s/%s-d%s", path_, tail, (line_cnt_ / MAX_LINES), suffix_);
        }
        locker.lock();
        Flush();
        fclose(fp_);
        fp_ = fopen(file_name, "a");
        assert(fp_ != nullptr);
    }
    {
        unique_lock<mutex> locker(mtx_);
        line_cnt_++;
        int n = snprintf(buff_.BeginWrite(), 128, "%04d-%02d-%02d %02d:%02d:%02d.%06ld ",
                        t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                        t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        buff_.HasWritten(n);
        AppendLogLevelTitle_(level);
        va_start(vl, format);
        int m = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, vl);
        va_end(vl);
        buff_.HasWritten(m);
        buff_.Append("\n\0", 2);
        if (is_async_ && deque_ && !deque_->IsFull()) {
            deque_->PushBack(buff_.RetrieveAllToStr());
        }
        else {
            fputs(buff_.Peek(), fp_);
        }
        buff_.RetrieveAll();
    }
}

void Log::Flush() {
    if (is_async_) {
        deque_->Flush();
    }
    fflush(fp_);
}

void Log::FlushLogThread() {
    Log::Instance()->AsyncWrite_();
}

Log* Log::Instance() {
    static Log inst;
    return &inst;
}

int Log::GetLevel() {
    lock_guard<mutex> locker(mtx_);
    return level_;
}

void Log::SetLevel(int level) {
    lock_guard<mutex> locker(mtx_);
    level_ = level;
}

bool Log::IsOpen() const {
    return is_open_;
}

void Log::AppendLogLevelTitle_(int level) {
    switch(level) {
    case 0:
        buff_.Append("[debug]: ", 9);
        break;
    case 1:
        buff_.Append("[info] : ", 9);
        break;
    case 2:
        buff_.Append("[warn] : ", 9);
        break;
    case 3:
        buff_.Append("[error]: ", 9);
        break;
    default:
        buff_.Append("[info] : ", 9);
        break;
    }
}

void Log::AsyncWrite_() {
    string str = "";
    while (deque_->Pop(str)) {
        lock_guard<mutex> locker(mtx_);
        fputs(str.c_str(), fp_);
    }
}
