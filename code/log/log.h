#include <string>
#include <thread>
#include <mutex>
#include <sys/time.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/stat.h>
#include "blockdeque.h"
#include "../buffer/buffer.h"


class Log {
private:
    Log();
    virtual ~Log();
    void AppendLogLevelTitle_(int level);
    void AsyncWrite_();
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;
    const char* path_;
    const char* suffix_;
    int line_cnt_;
    int to_day_;
    bool is_open_;
    Buffer buff_;
    int level_;
    bool is_async_;
    FILE* fp_;
    std::unique_ptr<BlockDeque<std::string>> deque_;
    std::unique_ptr<std::thread> write_thread_;
    std::mutex mtx_;
public:
    void Init(int level = 1, const char* path = "./log", 
                const char* suffix = ".log",
                int max_queue_capacity = 1024);
    void Write(int level, const char* format, ...);
    void Flush();
    static void FlushLogThread();
    static Log* Instance();
    int GetLevel();
    void SetLevel(int level);
    bool IsOpen() const;
};

#define LOG_BASE(level, format, ...)\
    do {\
        Log* log = Log::Instance();\
        if (log->IsOpen() && log->GetLevel() <= level) {\
            log->Write(level, format, ##__VA_ARGS__);\
            log->Flush();\
        }\
    } while (0);
#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while (0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while (0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while (0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while (0);
