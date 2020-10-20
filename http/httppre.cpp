#include "httpreq.h"
using namespace std;


const unordered_set<string> HttpReq::DEFAULT_HTML {
    "/index", "/register", "/login", "/welcome", "/video", "/picture"
};

const unordered_map<string, int> HttpReq::DEFAULT_HTML_TAG {
    {"/register.html", 0},
    {"/login.html", 1}
};

void HttpReq::Init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HttpReq::Parse(Buffer& buff) {
    const char CRLF[] = "\r\n";
    if (buff.ReadableBytes() <= 0) {
        return false;
    }
    while (buff.ReadableBytes() && state_ != FINISH) {
        const char* line_end = search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);
        string line(buff.Peek(), line_end);
        switch (state_) {
        case REQUEST_LINE:
            if (!ParseReqLine_(line)) {
                return false;
            }
            ParsePath_();
            break;
        case HEADER:
            ParseHeader_(line);
            if (buff.ReadableBytes() <= 2) {
                state_ = FINISH;
            }
            break;
        case BODY:
            ParseBody_(line);
            break;
        default:
            break;
        }
        if (line_end == buff.BeginWrite()) {
            break;
        }
        buff.RetrieveUntil(line_end + 2);
    }
    return true;
}

string HttpReq::path() const{
    return path_;
}

string& HttpReq::path(){
    return path_;
}
string HttpReq::method() const {
    return method_;
}

string HttpReq::version() const {
    return version_;
}

string HttpReq::GetPost(const string& key) const {
    assert(key != "");
    if (post_.count(key)) {
        return post_.find(key)->second;
    }
    return "";
}

string HttpReq::GetPost(const char* key) const {
    assert(key != nullptr);
    if(post_.count(key)) {
        return post_.find(key)->second;
    }
    return "";
}

bool HttpReq::IsKeepAlive() const {
    if (header_.count("Connection")) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpReq::ParseReqLine_(const std::string& text) {
    std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch matchRes;
    if (std::regex_match(text, matchRes, pattern)) {
        method_ = matchRes[1];
        path_ = matchRes[2];
        version_ = matchRes[3];
        state_ = HEADER;
        return true;
    }
    return false;
}

void HttpReq::ParseHeader_(const string& line) {
    regex patten("^([^:]*): ?(.*)$");
    smatch matchRes;
    if(regex_match(line, matchRes, patten)) {
        header_[matchRes[1]] = matchRes[2];
    }
    else {
        state_ = BODY;
    }
}

void HttpReq::ParseBody_(const string& line) {
    body_ = line;
    ParsePost_();
    state_ = FINISH;
}

void HttpReq::ParsePost_() {
    if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        ParseFromUrlencoded_();
        if (DEFAULT_HTML_TAG.count(path_)) {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            if (tag == 0 || tag == 1) {
                bool is_login = (tag == 1);
                /* if (UserVerify_(post_["username"], post_["password"], is_login)) {
                    path_ = "/welcome.html";
                } 
                else {
                    path_ = "/error.html";
                } */
                if (is_login) {
                    path_ = "/welcome.html";
                } 
                else {
                    path_ = "/error.html";
                }
            }
        }
    }   
}

void HttpReq::ParsePath_() {
    if (path_ == "/") {
        path_ = "/index.html"; 
    }
    else {
        for (auto &item: DEFAULT_HTML) {
            if (item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

void HttpReq::ParseFromUrlencoded_() {
    int n = body_.size();
    if (!n) {
        return;
    }
    string key = "", value = "";
    int num = 0;
    int i = 0, j = 0;
    for (; i < n; i++) {
        char c = body_[i];
        switch (c) {
        case '=':
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            body_[i] = ' ';
            break;
        case '%':
            num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
            body_[i + 2] = num % 10 + '0';
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    if (!post_.count(key) && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

int HttpReq::ConverHex(char c) {
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return c;
}
