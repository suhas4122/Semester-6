#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "rsocket.h"

#define PORT 110067
#define PORT1 110066

int main() {
    int sockfd = r_socket(AF_INET, SOCK_MRP, 0);
    struct sockaddr_in addr, server_addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    r_bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT1);
    inet_aton("127.0.0.1", &server_addr.sin_addr);

    char buff[1024];
    socklen_t socklen;
    while (1) {
        int len = r_recvfrom(sockfd, buff, 1024, 0, (struct sockaddr *)&server_addr, &socklen);
        printf("%.*s", len, buff);
        fflush(stdout);
    }
}