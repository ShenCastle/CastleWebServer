#include "httpres.h"
using namespace std;


const unordered_map<string, string> HttpRes::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "}
};

const unordered_map<int, string> HttpRes::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" }
};

const unordered_map<int, string> HttpRes::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" }
};

HttpRes::HttpRes() {
    code_ = -1;
    path_ = src_dir_ = "";
    is_keep_alive_ = false;
    file_ = nullptr;
    file_stat_ = { 0 };
}

HttpRes::~HttpRes() {
    UnmapFile();
}

void HttpRes::Init(const std::string& src_dir, const std::string& path, bool is_keep_alive, int code) {
    assert(src_dir != "");
    if (file_) {
        UnmapFile();
    }
    code_ = code;
    is_keep_alive_ = is_keep_alive;
    path_ = path;
    src_dir_ = src_dir;
    file_ = nullptr;
    file_stat_ = { 0 };
}

void HttpRes::Generate(Buffer& buff) {
    if (stat((src_dir_ + path_).data(), &file_stat_) < 0 || S_ISDIR(file_stat_.st_mode)) {
        code_ = 404;
    }
    else if (!(file_stat_.st_mode & S_IROTH)) {
        code_ = 403;
    }
    else if (code_ == -1) {
        code_ = 200;
    }
    ErrorHtml_();
    AddStatusLine_(buff);
    AddHeader_(buff);
    AddBody_(buff);
}

void HttpRes::UnmapFile() {
    if (file_) {
        munmap(file_, file_stat_.st_size);
        file_ = nullptr;
    }
}

char* HttpRes::file() const {
    return file_;
}

size_t HttpRes::FileLen() const {
    return file_stat_.st_size;
}

int HttpRes::code() const {
    return code_;
}

void HttpRes::ErrorHtml_() {
    if (CODE_PATH.count(code_)) {
        path_ = CODE_PATH.find(code_)->second;
        stat((src_dir_ + path_).data(), &file_stat_);
    }
}

void HttpRes::AddStatusLine_(Buffer& buff) {
    string status = "";
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    }
    else {
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff.Append("HTTP/1.1 " + to_string(code_) + " " + status + "\r\n");
}

void HttpRes::AddHeader_(Buffer& buff) {
    buff.Append("Connection: ");
    if(is_keep_alive_) {
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max=6, timeout=120\r\n");
    } else{
        buff.Append("close\r\n");
    }
    buff.Append("Content-type: " + GetFileType_() + "\r\n");
}

void HttpRes::AddBody_(Buffer& buff) {
    int srcfd = open((src_dir_ + path_).data(), O_RDONLY);
    if (srcfd < 0) {
        ErrorBody_(buff, "File Not Found!");
        return;
    }
    int* ret = (int*)mmap(0, file_stat_.st_size, PROT_READ, MAP_PRIVATE, srcfd, 0);
    if (*ret == -1) {
        ErrorBody_(buff, "File Not Found!");
        return;
    }
    file_ = (char*)ret;
    close(srcfd);
    buff.Append("Content-length: " + to_string(file_stat_.st_size) + "\r\n\r\n");
}

string HttpRes::GetFileType_() {
    string::size_type idx = path_.find_last_of('.');
    if (idx == string::npos) {
        return "text/plain";
    }
    string suffix = path_.substr(idx);
    if (SUFFIX_TYPE.count(suffix)) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

void HttpRes::ErrorBody_(Buffer& buff, const string& message) {
    string body = "", status = "";
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(code_)) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        status = "Bad Request";
    }
    body += to_string(code_) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>CastleWebServer</em></body></html>";
    buff.Append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}
