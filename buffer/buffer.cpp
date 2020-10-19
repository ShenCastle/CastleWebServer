#include "buffer.h"


Buffer::Buffer(int init_buff_size): buffer_(init_buff_size), read_pos_(0), write_pos_(0) {};

size_t Buffer::WritableBytes() const {
    return buffer_.size() - write_pos_;
}

size_t Buffer::ReadableBytes() const {
    return write_pos_ - read_pos_;
}

size_t Buffer::PrependableBytes() const {
    return read_pos_;
}

const char* Buffer::Peek() const {
    return BeginPtr_() + read_pos_;
}

void Buffer::EnsureWritable(size_t len) {
    if (WritableBytes() < len) {
        MakeSpace_(len);
    }
    assert(len <= WritableBytes());
}

void Buffer::HasWritten(size_t len) {
    write_pos_ += len;
}

void Buffer::Retrieve(size_t len) {
    assert(len <= ReadableBytes());
    read_pos_ += len;
}

void Buffer::RetrieveUntil(const char* end) {
    assert(end >= Peek());
    Retrieve(end - Peek());
}

void Buffer::RetrieveAll() {
    bzero(&buffer_[0], buffer_.size());
    read_pos_ = 0;
    write_pos_ = 0;
}

std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

const char* Buffer::BeginWriteConst() const {
    return BeginPtr_() + write_pos_;
}

char* Buffer::BeginWrite() {
    return BeginPtr_() + write_pos_;
}

void Buffer::Append(const std::string& str) {
    Append(str.data(), str.length());
}

void Buffer::Append(const char* str, size_t len) {
    assert(str);
    EnsureWritable(len);
    std::copy(str, str + len, BeginWrite());
    HasWritten(len);
}

void Buffer::Append(const void* data, size_t len) {
    assert(data);
    Append(static_cast<const char*>(data), len);
}

void Buffer::Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

ssize_t Buffer::ReadFd(int fd, int* errno_) {
    char buff[65535];
    iovec iov[2];
    const size_t writable = WritableBytes();
    iov[0].iov_base = BeginPtr_() + write_pos_;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);
    const ssize_t len = readv(fd, iov, 2);
    if (len < 0) {
        *errno_ = errno;
    }
    else if (static_cast<size_t>(len) <= writable) {
        write_pos_ += len;
    }
    else {
        write_pos_ = buffer_.size();
        Append(buff, len - writable);
    }
    return len;
}

ssize_t Buffer::WriteFd(int fd, int* errno_) {
    size_t readable = ReadableBytes();
    ssize_t len = write(fd, Peek(), readable);
    if (len < 0) {
        *errno_ = errno;
    }
    else {
        read_pos_ += len;
    }
    return len;
}

char* Buffer::BeginPtr_() {
    return &*buffer_.begin();
}

const char* Buffer::BeginPtr_() const {
    return &*buffer_.begin();
}

void Buffer::MakeSpace_(size_t len) {
    if (WritableBytes() + PrependableBytes() < len) {
        buffer_.resize(write_pos_ + len + 1);
    }
    else {
        size_t readable = ReadableBytes();
        std::copy(BeginPtr_() + read_pos_, BeginPtr_() + write_pos_, BeginPtr_());
        read_pos_ = 0;
        write_pos_ = read_pos_ + readable;
        assert(readable == ReadableBytes());
    }
}
