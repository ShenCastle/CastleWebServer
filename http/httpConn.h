#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <regex>
#include <string>
#include <unordered_map>


class HttpConn
{
private:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_UNFIN
    };
    int sockfd;
    sockaddr_in address;
    char readBuffer[READ_BUFFER_SIZE];
    int readIdx;
    int checkedIdx;
    int startLine;
    char writeBuffer[WRITE_BUFFER_SIZE];
    int writeIdx;
    CHECK_STATE checkState;
    METHOD method;
    std::string url;
    std::string version;
    std::unordered_map<std::string, std::string> header;
    std::string reqBody;
    /* std::string host;
    int contentLen;
    bool linger; */
    std::string fileName;
    std::string fileAddr;
    struct stat fileStat;
    struct iovec iv[2];
    int ivCount;
    int cgi;
    int bytesToSend;
    int bytesHaveSent;
    std::string docRoot;
    std::unordered_map<std::string, std::string> users;
    int trigMode;
    int closeLog;
    void addfd(int epollfd, int fd, bool oneShot);
    void delfd(int epollfd, int fd);
    void modfd(int epollfd, int fd, int ev);
    void init();
    HTTP_CODE processRead();
    bool processWrite(HTTP_CODE res);
    HTTP_CODE parseReqLine(const std::string& text);
    HTTP_CODE parseHeader(const std::string& text);
    HTTP_CODE parseBody(const std::string& text);
    LINE_STATUS parseLine();
    HTTP_CODE handleReq();
    void unmap();
    bool addRes(const std::string& format, ...);
    bool addContent(const std::string& content);
    bool addStatusLine(int status, const std::string& title);
    bool addHeader(int contenLength);
    bool addContentType();
    bool addContentLength(int contentLength);
    bool addLinger();
    bool addBlankLine();
public:
    HttpConn() {}
    ~HttpConn() {}
    void init(int sockfd, const sockaddr_in& addr);
    void closeConn(bool realClose = true);
    void process();
    bool read();
    bool write();
    sockaddr_in* getAddr();
    int timerFlag;
    int improv;
    static int epollfd;
    static int userCount;
    int state;
};
