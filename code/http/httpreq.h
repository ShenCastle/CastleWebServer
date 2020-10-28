#ifndef HTTP_REQ_H
#define HTTP_REQ_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <regex>
#include <error.h>
#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnpool.h"


class HttpReq
{
private:
    bool ParseReqLine_(const std::string& line);
    void ParseHeader_(const std::string& line);
    void ParseBody_(const std::string& line);
    void ParsePost_();
    void ParsePath_();
    void ParseFromUrlencoded_();
    static bool UserVerify(const std::string& name, const std::string& password, bool is_login);
    static int ConverHex(char c);
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADER,
        BODY,
        FINISH   
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
    PARSE_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;
    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
public:
    HttpReq() { Init(); }
    ~HttpReq() = default;
    void Init();
    bool Parse(Buffer& buff);
    std::string GetPath() const;
    std::string& GetPath();
    std::string GetMethod() const;
    std::string GetVersion() const;
    std::string GetPost(const std::string& key) const;
    std::string GetPost(const char* key) const;
    bool IsKeepAlive() const;
};

#endif
