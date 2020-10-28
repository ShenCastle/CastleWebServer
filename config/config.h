#include <getopt.h>
#include <stdlib.h>


class Config {
public:
    Config();
    ~Config() = default;
    void ParseArg(int argc, char* argv[]);
    int port;
    int trig_mode;
    int timeout;
    bool open_linger;
    int sql_port;
    char* sql_user;
    char* sql_password;
    char* db;
    int conn_num;
    int thread_num;
    bool open_log;
    int log_level;
    int log_queue_size;
};
