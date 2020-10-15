#include "httpConn.h"


bool HttpConn::read() {
    if (readIdx >= READ_BUFFER_SIZE) {
        return false;
    }
    int bytesRead = 0;
    if (trigMode == 1) {
        while (true) {
            bytesRead = recv(sockfd, &readBuffer[readIdx], READ_BUFFER_SIZE - readIdx, 0);
            if (bytesRead == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                return false;
            }
            else if (bytesRead == 0) {
                return false;
            }
            readIdx += bytesRead;
        }
        return true;
    }
    else {
        bytesRead = recv(sockfd, &readBuffer[readIdx], READ_BUFFER_SIZE - readIdx, 0);
        readIdx += bytesRead;
        if (bytesRead <= 0) {
            return false;
        }
        return true;
    }
}
