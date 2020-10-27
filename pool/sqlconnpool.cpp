#include "sqlconnpool.h"
using namespace std;


SqlConnPool::~SqlConnPool() {
    Close();
}

SqlConnPool* SqlConnPool::Instance() {
    static SqlConnPool conn_pool;
    return &conn_pool;
}

void SqlConnPool::Init(const char* host, int port,
                       const char* user, const char* password,
                       const char* db, int conn_num) {
    assert(conn_num > 0);
    for (int i = 0; i < conn_num; i++) {
        MYSQL* sql = nullptr;
        sql = mysql_init(sql);
        if (!sql) {
            LOG_ERROR("MySql init error!");
            assert(sql);
        }
        sql = mysql_real_connect(sql, host, user, password, db, port, nullptr, 0);
        if (!sql) {
            LOG_ERROR("MySql Connect error!");
        }
        conn_queue_.push(sql);
    }
    max_conn_ = conn_num;
    sem_init(&sem_, 0, max_conn_);
}

void SqlConnPool::Close() {
    lock_guard<mutex> locker(mtx_);
    while (!conn_queue_.empty()) {
        MYSQL* sql = conn_queue_.front();
        conn_queue_.pop();
        mysql_close(sql);
    }
    mysql_library_end();
}

MYSQL* SqlConnPool::GetConn() {
    MYSQL* sql = nullptr;
    if (conn_queue_.empty()) {
        LOG_WARN("SqlConnPool is busy!");
        return nullptr;
    }
    sem_wait(&sem_);
    {
        lock_guard<mutex> locker(mtx_);
        sql = conn_queue_.front();
        conn_queue_.pop();
    }
    return sql;
}

void SqlConnPool::FreeConn(MYSQL* sql) {
    assert(sql);
    lock_guard<mutex> locker(mtx_);
    conn_queue_.push(sql);
    sem_post(&sem_);
}

int SqlConnPool::GetFreeConnCount() {
    lock_guard<mutex> locker(mtx_);
    return conn_queue_.size();
}

SqlConnRAII::SqlConnRAII(MYSQL** sql, SqlConnPool* connpool) {
    assert(connpool);
    *sql = connpool->GetConn();
    sql_ = *sql;
    connpool_ = connpool;
}

SqlConnRAII::~SqlConnRAII() {
    if (sql_) {
        connpool_->FreeConn(sql_);
    }
}
