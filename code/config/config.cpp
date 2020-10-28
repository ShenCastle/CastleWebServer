#include "config.h"


Config::Config() {
    port = 1316;
    trig_mode = 3;
    timeout = 60000;
    open_linger = true;
    sql_port = 3306;
    sql_user = "root";
    sql_password = "root";
    db = "webserver";
    conn_num = 12;
    thread_num = 8;
    open_log = true;
    log_level = 0;
    log_queue_size = 1024;
}

void Config::ParseArg(int argc, char* argv[]) {
    int opt;
    const char* str = "p:m:l:s:t:o:";
    while ((opt = getopt(argc, argv, str)) != -1) {
        switch (opt) {
        case 'p':
            port = atoi(optarg);
            break;
        case 'm':
            trig_mode = atoi(optarg);
            break;
        case 'l':
            open_linger = atoi(optarg);
            break;
        case 's':
            conn_num = atoi(optarg);
            break;
        case 't':
            thread_num = atoi(optarg);
            break;
        case 'o':
            open_log = atoi(optarg);
            break;
        default:
            break;
        }
    }
}
