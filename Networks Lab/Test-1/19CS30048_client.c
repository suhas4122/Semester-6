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
#include <time.h>
#include <unistd.h>

#define PORT 20200
#define BUFF_SIZE 100

typedef struct client_information {
    struct in_addr IP;
    unsigned short int Port;
} client_information;

int n = 1;
const char *msg = "Message ";
client_information ci;

int main() {
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    int ret = fcntl(tcp_socket, F_SETFL, O_NONBLOCK);
    if (ret == -1) {
        perror("fcntl");
        exit(1);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &serv_addr.sin_addr);
    serv_addr.sin_port = htons(PORT);

    ret = connect(tcp_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (ret == -1) {
        perror("connect");
    }

    while (1) {
        srand(time(NULL));
        sleep(rand() % 3 + 1);

        if (n < 6) {
            ret = send(tcp_socket, msg, strlen(msg) + 1, 0);
            if (ret == -1) {
                perror("send");
            }
            int nbo = htonl(n);
            ret = send(tcp_socket, &nbo, sizeof(nbo), 0);
            if (ret == -1) {
                perror("send");
            }
            printf("%s%d sent\n", msg, n);
            n++;
        }

        ret = recv(tcp_socket, &ci.IP, sizeof(ci.IP), MSG_WAITALL);
        if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            else
                exit(EXIT_FAILURE);
        }
        ret = recv(tcp_socket, &ci.Port, sizeof(ci.Port), MSG_WAITALL);
        if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            else
                exit(EXIT_FAILURE);
        }
        char *sender_IP = inet_ntoa(ci.IP);
        unsigned short int sender_PORT = ntohs(ci.Port);

        char buffer[BUFF_SIZE];
        int message_number;
        int total_bytes = 0;
        int flag = 1;

        while (flag) {
            total_bytes += recv(tcp_socket, buffer + total_bytes, sizeof(char), 0);
            if (buffer[total_bytes - 1] == '\0') {
                flag = 0;
            }
        }

        ret = recv(tcp_socket, &message_number, sizeof(message_number), MSG_WAITALL);
        if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            else
                exit(EXIT_FAILURE);
        }

        message_number = ntohl(message_number);
        printf("Client: Received \"%s%d\" from client IP:%s PORT:%d\n", buffer, message_number, sender_IP, sender_PORT);
    }

    return 0;
}

/*
    Also,  write  as  a  comment  at  the  end  of  the  client  file  how  would  you  change  the  client  
    code  if  the  client  did  not  use  non-blocking  receive  AND  read  the  messages  to  be  sent 
    from the keyboard (to be entered by the user) instead of generating fixed messages after 
    random sleep. 
*/

/*
    Answer: A possible solution is to use select() to check if there is any data to be read and timeout after 
    a certain time. If there is data to be read, then read it. If there is no data to be read, then continue.
    If we have to take the input from user then we will not wait randomly for 1-3 secs, we'll just take a string 
    input and the random delay will be replaced by the time user takes to input the message.
*/
