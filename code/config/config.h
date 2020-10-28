#ifndef CONFIG_H
#define CONFIG_H

#include <getopt.h>
#include <stdlib.h>
#include <string>


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
    std::string sql_user;
    std::string sql_password;
    std::string db;
    int conn_num;
    int thread_num;
    bool open_log;
    int log_level;
    int log_queue_size;
};

#endif
