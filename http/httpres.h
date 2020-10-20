#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "../buffer/buffer.h"


class HttpRes {
private:
    void ErrorHtml_();
    void AddStatusLine_(Buffer& buff);
    void AddHeader_(Buffer& buff);
    void AddBody_(Buffer& buff);
    std::string GetFileType_();
    void ErrorBody_(Buffer& buff, const std::string& message);
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;
    int code_;
    bool is_keep_alive_;
    std::string path_;
    std::string src_dir_;
    char* file_;
    struct stat file_stat_;
public:
    HttpRes();
    ~HttpRes();
    void Init(const std::string& src_dir, const std::string& path, bool is_keep_alive = false, int code = -1);
    void Generate(Buffer& buff);
    void UnmapFile();
    char* file() const;
    size_t FileLen() const;
    int code() const;
};
