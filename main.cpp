#include <unistd.h>
#include "server/webserver.h"


int main() {
    /* 守护进程 后台运行 */
    // daemon(1, 0);
    /**
     * 端口 ET模式 timeoutMs 优雅退出
     * MySql: 端口 用户名 密码 数据库名
     * 连接池数量 线程池数量 日志开关 日志等级 日志异步队列容量
    */
    WebServer server(
        1316, 3, 60000, true,
        3306, "root", "root", "webserver",
        12, 6, true, 0, 1024);
    server.Start();
}
