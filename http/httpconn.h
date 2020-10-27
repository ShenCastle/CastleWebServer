#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <error.h>
#include "../log/log.h"
#include "../buffer/buffer.h"
#include "httpreq.h"
#include "httpres.h"


class HttpConn
{
private:
    int fd_;
    struct sockaddr_in addr_;
    bool is_close_;
    int iov_cnt_;
    struct iovec iov_[2];
    Buffer read_buff_;
    Buffer write_buff_;
    HttpReq request_;
    HttpRes response_;
public:
    HttpConn();
    ~HttpConn();
    void Init(int fd, const sockaddr_in& addr);
    void Close();
    bool Process();
    ssize_t Read(int* errno_);
    ssize_t Write(int* errno_);
    int GetFd() const;
    const sockaddr_in GetAddr() const;
    const char* GetIP() const;
    int GetPort() const;
    size_t ToWriteBytes() const;
    bool IsKeepAlive() const;
    static bool is_ET;
    static const char* src_dir;
    static std::atomic<int> user_cnt;
};
