#include <unistd.h>
#include "config/config.h"
#include "server/webserver.h"


int main(int argc, char* argv[]) {
    /* 守护进程 后台运行 */
    // daemon(1, 0);
    Config config;
    config.ParseArg(argc, argv);
    /**
     * 端口 ET模式 timeoutMs 优雅退出
     * MySql: 端口 用户名 密码 数据库名
     * 连接池数量 线程池数量
     * 日志开关 日志等级 日志异步队列容量
    */
    WebServer server(
        config.port, config.trig_mode, config.timeout, config.open_linger,
        config.sql_port, config.sql_user, config.sql_password, config.db,
        config.conn_num, config.thread_num,
        config.open_log, config.log_level, config.log_queue_size);
    server.Start();
    return 0;
}
