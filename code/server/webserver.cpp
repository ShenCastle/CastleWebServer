#include "webserver.h"
using namespace std;


WebServer::WebServer(
            int port, int trig_mode, int timeout, bool open_linger,
            int sql_port, const string& sql_user, const string& sql_password,
            const string& db, int conn_num, int thread_num,
            bool open_log, int log_level, int log_queque_size):
            port_(port), open_linger_(open_linger), timeout_(timeout), is_close_(false),
            timer_(new HeapTimer()), threadpool_(new ThreadPool(thread_num)), epoller_(new Epoller()) {
    src_dir_ = getcwd(nullptr, 256);
    assert(src_dir_);
    strncat(src_dir_, "/resources", 16);
    HttpConn::user_cnt = 0;
    HttpConn::src_dir = src_dir_;
    SqlConnPool::Instance()->Init("localhost", sql_port, sql_user.c_str(), sql_password.c_str(), db.c_str(), conn_num);
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
            LOG_INFO("Port: %d, OpenLinger: %s", port_, open_linger_ ? "true" : "false");
            LOG_INFO("Listen Mode: %s, Connect Mode: %s",
                    (listen_event_ & EPOLLET ? "ET" : "LT"),
                    (connect_event_ & EPOLLET ? "ET" : "LT"));
            LOG_INFO("Log Level: %d", log_level);
            LOG_INFO("SrcDir: %s", src_dir_);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", conn_num, thread_num);
        }
    }
}

WebServer::~WebServer() {
    close(listen_fd_);
    is_close_ = true;
    free(src_dir_);
    SqlConnPool::Instance()->Close();
}

void WebServer::Start() {
    int timeout = -1;
    if (!is_close_) {
        LOG_INFO("========== Server Start ==========");
    }
    while (!is_close_) {
        if (timeout_ > 0) {
            timeout = timer_->GetNextTick();
        }
        int event_cnt = epoller_->Wait(timeout);
        for (int i = 0; i < event_cnt; i++) {
            int fd = epoller_->GetEventFd(i);
            uint32_t events = epoller_->GetEvents(i);
            if (fd == listen_fd_) {
                HandleListen_();
            }
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                CloseConn_(&users_[fd]);
            }
            else if (events & EPOLLIN) {
                assert(users_.count(fd) > 0);
                HandleRead_(&users_[fd]);
            }
            else if (events & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                HandleWrite_(&users_[fd]);
            }
            else {
                LOG_ERROR("Unexpected event!");
            }
        }
    }
}

void WebServer::InitEventMode_(int trig_mode) {
    listen_event_ = EPOLLRDHUP;
    connect_event_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trig_mode) {
    case 0:
        break;
    case 1:
        connect_event_ |= EPOLLET;
        break;
    case 2:
        listen_event_ |= EPOLLET;
        break;
    case 3:
        listen_event_ |= EPOLLET;
        connect_event_ |= EPOLLET;
        break;
    default:
        listen_event_ |= EPOLLET;
        connect_event_ |= EPOLLET;
        break;
    }
    HttpConn::is_ET = (connect_event_ & EPOLLET);
}

bool WebServer::InitSocket_() {
    if (port_ > 65535 || port_ < 1024) {
        LOG_ERROR("Port: %d error!", port_);
        return false;
    }
    struct linger opt_linger = {0};
    if (open_linger_) {
        opt_linger.l_onoff = 1;
        opt_linger.l_linger = 1;
    }
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        LOG_ERROR("Create socket error!");
        return false;
    }
    int ret = setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER, (const void*)&opt_linger, sizeof(opt_linger));
    if (ret == -1) {
        LOG_ERROR("Set linger error!");
        close(listen_fd_);
        return false;
    }
    int opt_val = 1;
    ret = setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt_val, sizeof(int));
    if (ret == -1) {
        LOG_ERROR("Set reuse address error!");
        close(listen_fd_);
        return false;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    ret = bind(listen_fd_, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        LOG_ERROR("Bind port: %d error!", port_);
        close(listen_fd_);
        return false;
    }
    ret = listen(listen_fd_, 6);
    if (ret < 0) {
        LOG_ERROR("Listen port: %d error!", port_);
        close(listen_fd_);
        return false;
    }
    ret = epoller_->AddFd(listen_fd_, listen_event_ | EPOLLIN);
    if (ret == 0) {
        LOG_ERROR("Add listen error!");
        close(listen_fd_);
        return false;
    }
    SetFdNonBlock(listen_fd_);
    LOG_INFO("Server port: %d", port_);
    return true;
}

void WebServer::HandleListen_() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listen_fd_, (struct sockaddr*)&addr, &len);
        if (fd <= 0) {
            return;
        }
        else if (HttpConn::user_cnt >= MAX_FD) {
            SendError_(fd, "Server is busy!");
            LOG_WARN("Client is full!");
            return;
        }
        AddClient_(fd, addr);
    } while (listen_fd_ & EPOLLET);
}

void WebServer::CloseConn_(HttpConn* client) {
    assert(client);
    LOG_INFO("Client[%d] quit", client->GetFd());
    epoller_->DelFd(client->GetFd());
    client->Close();
}

void WebServer::HandleRead_(HttpConn* client) {
    assert(client);
    ExtendTime_(client);
    threadpool_->AddTask(std::bind(&WebServer::OnRead_, this, client));
}

void WebServer::HandleWrite_(HttpConn* client) {
    assert(client);
    ExtendTime_(client);
    threadpool_->AddTask(std::bind(&WebServer::OnWrite_, this, client));
}

void WebServer::AddClient_(int fd, struct sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].Init(fd, addr);
    if (timeout_ > 0) {
        timer_->Add(fd, timeout_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));
    }
    epoller_->AddFd(fd, connect_event_ | EPOLLIN);
    SetFdNonBlock(fd);
    LOG_INFO("Client[%d] in", users_[fd].GetFd());
}

void WebServer::SendError_(int fd, const char* info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if (ret == -1) {
        LOG_WARN("Send error to client[%d] error!", fd);
    }
    close(fd);
}

void WebServer::ExtendTime_(HttpConn* client) {
    assert(client);
    if (timeout_ > 0) {
        timer_->Adjust(client->GetFd(), timeout_);
    }
}

void WebServer::OnRead_(HttpConn* client) {
    assert(client);
    int read_errno = 0;
    int ret = client->Read(&read_errno);
    if (ret <= 0 && read_errno != EAGAIN) {
        CloseConn_(client);
        return;
    }
    OnProcess(client);
}

void WebServer::OnWrite_(HttpConn* client) {
    assert(client);
    int write_errno = 0;
    int ret = client->Write(&write_errno);
    if (client->ToWriteBytes() == 0) {
        if (client->IsKeepAlive()) {
            OnProcess(client);
            return;
        }
    }
    else if (ret < 0) {
        if (write_errno == EAGAIN) {
            epoller_->ModFd(client->GetFd(), connect_event_ | EPOLLOUT);
            return;
        }
    }
    CloseConn_(client);
}

void WebServer::OnProcess(HttpConn* client) {
    assert(client);
    if (client->Process()) {
        epoller_->ModFd(client->GetFd(), connect_event_ | EPOLLOUT);
    }
    else {
        epoller_->ModFd(client->GetFd(), connect_event_ | EPOLLIN);
    }
}

int WebServer::SetFdNonBlock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}
