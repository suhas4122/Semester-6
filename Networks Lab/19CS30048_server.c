/********************************************************/
/* Multicast Chat Application Using Stream Sockets      */
/* Suhas Jain                                           */
/* 19CS30048                                            */
/* Lab Test - 1                                         */
/********************************************************/

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define SERVER_PORT 20200
#define BUFF_SIZE 100

typedef struct client_information {
    struct in_addr IP;
    unsigned short int Port;
} client_information;

int clientsockfd[] = {-1, -1, -1};
client_information clientId[3];
int numclient = 0;

void accept_new_client(int server_socket) {
    struct sockaddr_in client_addr;
    socklen_t cli_len = sizeof(client_addr);
    clientsockfd[numclient] = accept(server_socket, (struct sockaddr*)&client_addr, &cli_len);
    struct in_addr client_IP = client_addr.sin_addr;
    unsigned short int client_PORT = client_addr.sin_port;
    clientId[numclient].IP = client_IP;
    clientId[numclient++].Port = client_PORT;
    printf("Server: Received a new connection from client IP:%s PORT:%d\n\n", inet_ntoa(client_IP), ntohs(client_PORT));
    return;
}

void receive_message(int client_no) {
    char buffer[BUFF_SIZE];
    int message;
    int total_bytes = 0;
    int flag = 1;
    char* client_IP = inet_ntoa(clientId[client_no].IP);
    unsigned short int client_PORT = ntohs(clientId[client_no].Port);

    while (flag) {
        total_bytes += recv(clientsockfd[client_no], buffer + total_bytes, sizeof(char), 0);
        if (buffer[total_bytes - 1] == '\0') {
            flag = 0;
        }
    }

    int ret = recv(clientsockfd[client_no], &message, sizeof(message), MSG_WAITALL);
    if (ret == -1) {
        perror("recv");
    }
    message = ntohl(message);

    printf("Server: Received \"%s%d\" from Client IP:%s PORT:%d\n\n", buffer, message, client_IP, client_PORT);

    if (numclient == 1) {
        printf("Server: Insufficient clients, \"%s%d\" from client IP:%s PORT:%d dropped\n\n", buffer, message, client_IP, client_PORT);
    } else {
        for (int j = 0; j < numclient; j++) {
            if (client_no != j) {
                struct in_addr IP = clientId[j].IP;
                unsigned short int PORT = clientId[j].Port;
                int ret = send(clientsockfd[j], &IP, sizeof(IP), 0);
                if (ret == -1) {
                    perror("send");
                }
                ret = send(clientsockfd[j], &PORT, sizeof(PORT), 0);
                if (ret == -1) {
                    perror("send");
                }
                ret = send(clientsockfd[j], buffer, total_bytes, 0);
                if (ret == -1) {
                    perror("send");
                }
                int nbo_message = ntohl(message);
                ret = send(clientsockfd[j], &nbo_message, sizeof(nbo_message), 0);
                if (ret == -1) {
                    perror("send");
                }
                char* client_IP2 = inet_ntoa(clientId[j].IP);
                unsigned short int client_PORT2 = ntohs(clientId[j].Port);
                printf("Server: Sent \"%s%d\" from client IP:%s PORT:%d", buffer, message, client_IP, client_PORT);
                printf("\nto client IP:%s PORT:%d\n\n", client_IP2, client_PORT2);
            }
        }
    }
    return;
}

int main() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    int enable = 1;
    int ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(SERVER_PORT);

    ret = bind(server_socket, (struct sockaddr*)&server, sizeof(server));

    listen(server_socket, 5);
    printf("Server is Running... \n");

    int i;
    while (1) {
        fd_set fds;

        FD_ZERO(&fds);

        FD_SET(server_socket, &fds);
        for (i = 0; i < numclient; i++) {
            if (clientsockfd[i] != -1)
                FD_SET(clientsockfd[i], &fds);
        }

        int nfds = server_socket;
        for (i = 0; i < numclient; i++) {
            if (clientsockfd[i] != -1)
                nfds = (nfds > clientsockfd[i]) ? nfds : clientsockfd[i];
        }

        ret = select(nfds + 1, &fds, NULL, NULL, NULL);

        if (FD_ISSET(server_socket, &fds)) {
            accept_new_client(server_socket);
        }
        for (i = 0; i < numclient; i++) {
            if (clientsockfd[i] != -1 && FD_ISSET(clientsockfd[i], &fds)) {
                receive_message(i);
            }
        }
    }
    return 0;
}