#include <string>
#include <unordered_map>
#include <unordered_set>
#include <regex>
#include <error.h>
#include "../buffer/buffer.h"


class HttpReq
{
private:
    bool ParseReqLine_(const std::string& line);
    void ParseHeader_(const std::string& line);
    void ParseBody_(const std::string& line);
    void ParsePost_();
    void ParsePath_();
    void ParseFromUrlencoded_();
    static bool UserVerify_(const std::string& name, const std::string& pwd, bool is_login);
    static int ConverHex(char ch);
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
    std::string path() const;
    std::string& path();
    std::string method() const;
    std::string version() const;
    std::string GetPost(const std::string& key) const;
    std::string GetPost(const char* key) const;
    bool IsKeepAlive() const;
};
