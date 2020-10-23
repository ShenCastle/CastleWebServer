#include "webserver.h"
using namespace std;


WebServer::WebServer(
            int port, int trig_mode, int timeout, bool linger,
            int thread_num, bool open_log, int log_level, int log_queque_size):
            port_(port), linger_(linger), timeout_(timeout), is_close_(false),
            timer_(new HeapTimer()), threadpool_(new ThreadPool(thread_num)), epoller_(new Epoller()) {
    src_dir_ = getcwd(nullptr, 256);
    assert(src_dir_);
    strncat(src_dir_, "/resources/", 16);
    HttpConn::user_cnt = 0;
    HttpConn::src_dir = src_dir_;
    InitEventMode_(trig_mode);
    if (!InitSocket_()) {
        is_close_ = true;
    }
    if (open_log) {
        Log::Instance()->Init(log_level, "./log", ".log", log_queque_size);
        if (is_close_) {
            LOG_ERROR("========== Server Init Error! ==========");
        }
        else {
            LOG_INFO("========== Server Init ==========");
            LOG_INFO("Port: %d, OpenLinger: %s", port_, linger_ ? "true" : "false");
            LOG_INFO("Listen Mode: %s, Connect Mode: %s",
                    (listen_event_ & EPOLLET ? "ET" : "LT"),
                    (connect_event_ & EPOLLET ? "ET" : "LT"));
            LOG_INFO("Log Level: %d", log_level);
            LOG_INFO("SrcDir: %s", src_dir_);
        }
    }
}

WebServer::~WebServer() {
    close(listenFd_);
    is_close_ = true;
    free(src_dir_);
}
