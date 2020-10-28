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
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

string HttpReq::GetPath() const{
    return path_;
}

string& HttpReq::GetPath(){
    return path_;
}
string HttpReq::GetMethod() const {
    return method_;
}

string HttpReq::GetVersion() const {
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
    LOG_ERROR("RequestLine Error!");
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
    LOG_DEBUG("Body: %s, len: %d", line.c_str(), line.size());
}

void HttpReq::ParsePost_() {
    if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        ParseFromUrlencoded_();
        if (DEFAULT_HTML_TAG.count(path_)) {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag: %d", tag);
            if (tag == 0 || tag == 1) {
                bool is_login = (tag == 1);
                if (UserVerify(post_["username"], post_["password"], is_login)) {
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
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
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

bool HttpReq::UserVerify(const string& name, const string& password, bool is_login) {
    if (name == "" || password == "") {
        return false;
    }
    LOG_INFO("Verify name: %s password: %s", name.c_str(), password.c_str());
    MYSQL* sql = nullptr;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    assert(sql);
    bool flag = false;
    char order[256] = {0};
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;
    if (!is_login) {
        flag = true;
    }
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);
    if (mysql_query(sql, order)) {
        mysql_free_result(res);
        return false;
    }
    res = mysql_store_result(sql);
    unsigned int fields_num = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);
    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MySql row: %s %s", row[0], row[1]);
        string pwd(row[1]);
        if (is_login) {
            if (pwd == password) {
                flag = true;
            }
            else {
                flag = false;
                LOG_DEBUG("Password error!");
            }
        }
        else {
            flag = false;
            LOG_DEBUG("Username used!");
        }
    }
    mysql_free_result(res);
    if (!is_login && flag) {
        LOG_DEBUG("Sign up!");
        bzero(order, 256);
        snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s', '%s')", name.c_str(), password.c_str());
        LOG_DEBUG("%s", order);
        if (mysql_query(sql, order)) {
            LOG_DEBUG("Insert error!");
            flag = false;
        }
    }
    // SqlConnPool::Instance()->FreeConn(sql);
    LOG_DEBUG("UserVerify completed!");
    return flag;
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
