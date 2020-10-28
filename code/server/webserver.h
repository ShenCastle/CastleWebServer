#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <string>
#include <unordered_map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include "epoller.h"
#include "../log/log.h"
#include "../timer/heaptimer.h"
#include "../pool/threadpool.h"
#include "../http/httpconn.h"
#include "../pool/sqlconnpool.h"


class WebServer {
private:
    void InitEventMode_(int trig_mode);
    bool InitSocket_();
    void HandleListen_();
    void CloseConn_(HttpConn* client);
    void HandleRead_(HttpConn* client);
    void HandleWrite_(HttpConn* client);
    void AddClient_(int fd, struct sockaddr_in addr);
    void SendError_(int fd, const char* info);
    void ExtendTime_(HttpConn* client);
    void OnRead_(HttpConn* client);
    void OnWrite_(HttpConn* client);
    void OnProcess(HttpConn* client);
    static int SetFdNonBlock(int fd);
    static const int MAX_FD = 65536;
    int port_;
    bool open_linger_;
    int timeout_;
    bool is_close_;
    int listen_fd_;
    char* src_dir_;
    uint32_t listen_event_;
    uint32_t connect_event_;
    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;
public:
    WebServer(
        int port, int trig_mode, int timeout, bool open_linger,
        int sql_port, const std::string& sql_user, const std::string& sql_password,
        const std::string& db, int conn_num, int thread_num,
        bool open_log, int log_level, int log_queque_size);
    ~WebServer();
    void Start();
};

#endif
