#include "httpConn.h"


void HttpConn::process() {
    HTTP_CODE readRes = processRead();
    if (readRes == NO_REQUEST) {
        modfd(epollfd, sockfd, EPOLLIN);
        return;
    }
    bool writeRes = processWrite(readRes);
    if (!writeRes) {
        closeConn();
    }
    modfd(epollfd, sockfd, EPOLLOUT);
}
HttpConn::HTTP_CODE HttpConn::processRead() {
    LINE_STATUS lineStatus = LINE_OK;
    HTTP_CODE res = NO_REQUEST;
    std::string text = "";
    while ((checkState == CHECK_STATE_CONTENT && lineStatus == LINE_OK) || (lineStatus = parseLine()) == LINE_OK) {
        text = std::string(readBuffer + startLine);
        checkedIdx = startLine;
        switch (checkState) {
            case CHECK_STATE_REQUESTLINE:
                res = parseReqLine(text);
                if (res == BAD_REQUEST) {
                    return BAD_REQUEST;
                }
                break;
            case CHECK_STATE_HEADER:
                res = parseHeader(text);
                if (res == BAD_REQUEST) {
                    return BAD_REQUEST;
                }
                else if (res == GET_REQUEST) {
                    return handleReq();
                }
                break;
            case CHECK_STATE_CONTENT:
                res = parseBody(text);
                if (res == GET_REQUEST) {
                    return handleReq();
                }
                lineStatus = LINE_UNFIN;
                break;
            default:
                return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}
HttpConn::LINE_STATUS HttpConn::parseLine() {
    char c;
    for (; checkedIdx < readIdx; checkedIdx++) {
        c = readBuffer[checkedIdx];
        if (c == '\r') {
            if (checkedIdx == readIdx - 1) {
                return LINE_UNFIN;
            }
            else if (readBuffer[checkedIdx + 1] == '\n') {
                readBuffer[checkedIdx++] = '\0';
                readBuffer[checkedIdx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if (c == '\n') {
            if (checkedIdx > 0 && readBuffer[checkedIdx - 1] == '\r') {
                readBuffer[checkedIdx - 1] = '\0';
                readBuffer[checkedIdx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_UNFIN;
}
HttpConn::HTTP_CODE HttpConn::parseReqLine(const std::string& text) {
    std::regex pattern("^([^ ]*) ([^ ]*) ([^ ]*)$");
    std::smatch matchRes;
    if (std::regex_match(text, matchRes, pattern)) {
        if (matchRes[1] == "GET") {
            method = GET;
        }
        else if (matchRes[1] == "POST") {
            method = POST;
            cgi = 1;
        }
        else {
            return BAD_REQUEST;
        }
        url = matchRes[2];
        if (url.empty()) {
            return BAD_REQUEST;
        }
        version = matchRes[3];
        if (version != "HTTP/1.1") {
            return BAD_REQUEST;
        }
        checkState = CHECK_STATE_HEADER;
        return NO_REQUEST;
    }
    return BAD_REQUEST;
}
HttpConn::HTTP_CODE HttpConn::parseHeader(const std::string& text) {
    if (text == "") {
        if (header.count("Content-length")) {
            checkState = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    std::regex pattern("^([^:]*): ?(.*)$");
    std::smatch matchRes;
    if (std::regex_match(text, matchRes, pattern)) {
        header[matchRes[1]] = matchRes[2];
        return NO_REQUEST;
    }
    return BAD_REQUEST;
}
HttpConn::HTTP_CODE HttpConn::parseBody(const std::string& text) {
    if (readIdx >= (checkedIdx + std::stoi(header["Content-length"]))) {
        reqBody = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}
HttpConn::HTTP_CODE HttpConn::handleReq() {
    fileName = docRoot + url;
    if (stat(fileName.data(), &fileStat) < 0) {
        return NO_RESOURCE;
    }
    if (!(fileStat.st_mode & S_IROTH)) {
        return FORBIDDEN_REQUEST;
    }
    if (S_ISDIR(fileStat.st_mode)) {
        return BAD_REQUEST;
    }
    int srcfd = open(fileName.data(), O_RDONLY);
    fileAddr = (char*)mmap(0, fileStat.st_size, PROT_READ, MAP_PRIVATE, srcfd, 0);
    close(srcfd);
    return FILE_REQUEST;
}
bool HttpConn::addRes(const std::string& format, ...) {
    if (writeIdx >= WRITE_BUFFER_SIZE) {
        return false;
    }
    va_list args;
    va_start(args, format);
    int len = vsnprintf(writeBuffer + writeIdx, WRITE_BUFFER_SIZE - writeIdx - 1, format.data(), args);
    if (len >= WRITE_BUFFER_SIZE - 1 - writeIdx) {
        va_end(args);
        return false;
    }
    writeIdx += len;
    va_end(args);
    return true;
}
bool HttpConn::addStatusLine(int status, const std::string& title) {
    return addRes("%s %d %s\r\n", "HTTP/1.1", status, title);
}
bool HttpConn::read() {
    if (readIdx >= READ_BUFFER_SIZE) {
        return false;
    }
    int bytesRead = 0;
    if (trigMode == 1) {
        while (true) {
            bytesRead = recv(sockfd, &readBuffer[readIdx], READ_BUFFER_SIZE - readIdx, 0);
            if (bytesRead == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                return false;
            }
            else if (bytesRead == 0) {
                return false;
            }
            readIdx += bytesRead;
        }
        return true;
    }
    else {
        bytesRead = recv(sockfd, &readBuffer[readIdx], READ_BUFFER_SIZE - readIdx, 0);
        readIdx += bytesRead;
        if (bytesRead <= 0) {
            return false;
        }
        return true;
    }
}
void HttpConn::addfd(int epollfd, int fd, bool oneShot) {
    epoll_event event;
    event.data.fd = fd;
    if (trigMode == 1) {
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    }
    else {
        event.events = EPOLLIN | EPOLLRDHUP;
    }
    if (oneShot) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    int oldOp = fcntl(fd, F_GETFL);
    int newOp = oldOp | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOp);
}
void HttpConn::delfd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}
void HttpConn::modfd(int epollfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    if (trigMode == 1) {
        event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    }
    else {
        event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    }
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}
