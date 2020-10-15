#include <netinet/in.h>
#include <vector>
#include <string>
#include <map>
#include <sys/stat.h>
#include <sys/uio.h>


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
        LINE_OPEN
    };
    int sockfd;
    sockaddr_in address;
    std::vector<char> readBuffer;
    int readIdx;
    int checkedIdx;
    int startLine;
    std::vector<char> writeBuffer;
    int writeIdx;
    CHECK_STATE checkState;
    METHOD method;
    std::vector<char> fileName;
    std::string url;
    std::string version;
    std::string host;
    int contentLen;
    bool linger;
    std::string fileAddr;
    struct stat fileStat;
    struct iovec iv[2];
    int ivCount;
    int cgi;
    std::string str;
    int bytesToSend;
    int bytesHaveSent;
    std::string docRoot;
    std::map<std::string, std::string> users;
    int trigMode;
    int closeLog;
    void init();
    HTTP_CODE processRead();
    bool processWrite(HTTP_CODE res);
    HTTP_CODE parseReqLine(std::string text);
    HTTP_CODE parseHeader(std::string text);
    HTTP_CODE parseContent(std::string text);
    LINE_STATUS parseLine();
    HTTP_CODE handleReq();
    void unmap();
    bool addRes(std::string format, ...);
    bool addContent(std::string content);
    bool addStatusLine(int status, std::string title);
    bool addHeader(int contenLength);
    bool addContentType();
    bool addContentLength(int contentLength);
    bool addLinger();
    bool addBlankLine();
public:
    HttpConn(): readBuffer(READ_BUFFER_SIZE), writeBuffer(WRITE_BUFFER_SIZE), fileName(FILENAME_LEN) {}
    ~HttpConn() {}
    void init(int sockfd, const sockaddr_in &addr);
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
