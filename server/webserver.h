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


class WebServer {
private:
    bool InitSocket_();
    void InitEventMode_(int trig_mode);
    void AddClient_(int fd, sockaddr_in addr);
    void HandleListen_();
    void HandleRead_(HttpConn* client);
    void HandleWrite_(HttpConn* client);
    void SendError_(int fd, const char* info);
    void ExtentTime_(HttpConn* client);
    void CloseConn_(HttpConn* client);
    void OnRead_(HttpConn* client);
    void OnWrite_(HttpConn* client);
    void OnProcess(HttpConn* client);
    static int SetFdNonBlock(int fd);
    static const int MAX_FD = 65536;
    int port_;
    bool linger_;
    int timeout_;
    bool is_close_;
    int listenFd_;
    char* src_dir_;
    uint32_t listen_event_;
    uint32_t connect_event_;
    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;
public:
    WebServer(
        int port, int trig_mode, int timeout, bool linger,
        int thread_num, bool open_log, int log_level, int log_queque_size);
    ~WebServer();
    void Start();
};
