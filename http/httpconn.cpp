#include "httpconn.h"
using namespace std;


const char* HttpConn::src_dir;

std::atomic<int> HttpConn::user_cnt;

bool HttpConn::is_ET;

HttpConn::HttpConn() {
    fd_ = -1;
    addr_ = { 0 };
    is_close_ = true;
}

HttpConn::~HttpConn() {
    Close();
}

void HttpConn::Init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    user_cnt++;
    addr_ = addr;
    fd_ = fd;
    read_buff_.RetrieveAll();
    write_buff_.RetrieveAll();
    is_close_ = false;
    LOG_INFO("Client[%d](%s: %d) in, user count: %d", fd_, GetIP(), GetPort(), int(user_cnt));
}

void HttpConn::Close() {
    response_.UnmapFile();
    if (!is_close_) {
        is_close_ = true;
        user_cnt--;
        close(fd_);
        LOG_INFO("Client[%d](%s: %d) quit, user count: %d", fd_, GetIP(), GetPort(), int(user_cnt));
    }
}

bool HttpConn::Process() {
    request_.Init();
    if (read_buff_.ReadableBytes() <= 0) {
        return false;
    }
    else if (request_.Parse(read_buff_)) {
        LOG_DEBUG("%s", request_.GetPath().c_str());
        response_.Init(src_dir, request_.GetPath(), request_.IsKeepAlive(), 200);
    }
    else {
        response_.Init(src_dir, request_.GetPath(), false, 400);
    }
    response_.Generate(write_buff_);
    iov_[0].iov_base = const_cast<char*>(write_buff_.Peek());
    iov_[0].iov_len = write_buff_.ReadableBytes();
    iov_cnt_ = 1;
    if (response_.GetFileLen() > 0 && response_.GetFile()) {
        iov_[1].iov_base = response_.GetFile();
        iov_[1].iov_len = response_.GetFileLen();
        iov_cnt_ = 2;
    }
    LOG_DEBUG("File size: %d, %d to %d", response_.GetFileLen(), iov_cnt_, ToWriteBytes());
    return true;
}

ssize_t HttpConn::Read(int* errno_) {
    ssize_t len = -1;
    do {
        len = read_buff_.ReadFd(fd_, errno_);
        if (len <= 0) {
            break;
        }
    } while (is_ET);
    return len;
}

ssize_t HttpConn::Write(int* errno_) {
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iov_cnt_);
        if (len <= 0) {
            *errno_ = errno;
            break;
        }
        if (!ToWriteBytes()) {
            break;
        }
        else if (static_cast<size_t>(len) > iov_[0].iov_len) {
            iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if (iov_[0].iov_len) {
                write_buff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }
        else {
            iov_[0].iov_base = (uint8_t*) iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            write_buff_.Retrieve(len);
        }
    } while (is_ET || ToWriteBytes() > 10240);
    return len;
}

int HttpConn::GetFd() const {
    return fd_;
};

const sockaddr_in HttpConn::GetAddr() const {
    return addr_;
}

const char* HttpConn::GetIP() const {
    return inet_ntoa(addr_.sin_addr);
}

int HttpConn::GetPort() const {
    return addr_.sin_port;
}

size_t HttpConn::ToWriteBytes() const {
    return iov_[0].iov_len + iov_[1].iov_len;
}

bool HttpConn::IsKeepAlive() const {
    return request_.IsKeepAlive();
}
