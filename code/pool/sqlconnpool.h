#include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include "../log/log.h"


class SqlConnPool {
private:
    SqlConnPool() = default;
    ~SqlConnPool();
    int max_conn_;
    std::queue<MYSQL*> conn_queue_;
    std::mutex mtx_;
    sem_t sem_;

public:
    static SqlConnPool* Instance();
    void Init(const char* host, int port,
              const char* user, const char* password,
              const char* db, int conn_num);
    void Close();
    MYSQL* GetConn();
    void FreeConn(MYSQL* conn);
    int GetFreeConnCount();
};

class SqlConnRAII {
private:
    MYSQL* sql_;
    SqlConnPool* connpool_;
public:
    SqlConnRAII(MYSQL** sql, SqlConnPool* connpool);
    ~SqlConnRAII();
};
