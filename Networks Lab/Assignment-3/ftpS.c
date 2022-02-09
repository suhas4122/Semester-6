#define _GNU_SOURCE  // for DT_DIR
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "mylib.h"

// flag for logged in
int flogged = 0;
char cmd[1024];                        // latest command
int cmdLen = 0;                        // command length
const char* fCredsName = "users.txt";  // credentials file
char* errString = "";

char* argv[1024];
int argc = 0;

/**
 * @brief Recieves a file from the socket specified and tokenises it
 *
 * @param sockfd socket from which to recieve
 * @return 1 if successfull, -1 if error, 0 if connection closed
 */
int recvCmd(int sockfd) {
    int n = 0;
    char prv = ' ';
    while (prv != '\0') {
        int r = recv(sockfd, cmd + n, 1024 - n, 0);
        if (r < 0 && errno != EINTR) {
            errString = strerror(errno);
            return -1;
        }
        if (r == 0) {
            errString = "Connection closed";
            return 0;
        }
        n += r;
        prv = cmd[n - 1];
    }
    cmdLen = n;
    INFO("Command: %s\n", cmd);
    char* cmdcpy = malloc(cmdLen);
    strcpy(cmdcpy, cmd);
    tokenize(cmdcpy);
    return 1;
}

/**
 * @brief Sends status code back to the client
 * 
 * @param sockfd socket to send to
 * @param code status code which needs to be sent 
 * @return int 0 if successfull, -1 if error
 */
int sendStatusCode(int sockfd, char* code) {
    INFO("Status code: %s\n", code);
    int len = strlen(code);
    int n = send(sockfd, code, len, 0);
    if (n < 0 && errno != EINTR) {
        errString = strerror(errno);
        return -1;
    }
    send(sockfd, "\0", 1, 0);
    return 0;
}

/**
 * @brief Handles the client 
 * 
 * @param sockfd client socket 
 * @param cli_addr struct that stores client IP and port
 */
void handleClient(int sockfd, struct sockaddr_in* cli_addr) {
    int PORT = ntohs(cli_addr->sin_port);
    char* IP = inet_ntoa(cli_addr->sin_addr);
    char logFile[1024], buff[1024];  // log filename

    sprintf(logFile, ".tmp_%s_%d.txt", IP, PORT);
    INFO("Connected to: %s:%d\n", IP, PORT);
    initLogger(logFile);  // toggle to switch to console logging

    while (!flogged) {
        int res;
        if ((res = recvCmd(sockfd)) < 0) {
            ERROR("%s\n", errString);
            return;
        }
        if (res == 0) {
            INFO("Connection closed by client\n");
            return;
        }
        if (argc < 2 || strcmp(argv[0], "user") != 0) {
            ERROR("User authentication req.\n");
            sendStatusCode(sockfd, STATUS_CMD);
            continue;
        }
        FILE* fd = fopen(fCredsName, "r");
        if (fd == NULL) {
            ERROR("Could not open credentials file: %s\n", strerror(errno));
            return;
        }
        char *user, *pass;  // user and password
        int found = 0;
        while (fscanf(fd, "%[^\n]s", buff) != EOF) {
            fgetc(fd);  // skip newline
            user = strtok(buff, " ");
            pass = strtok(NULL, " ");
            if (strcmp(user, argv[1]) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            INFO("Username not found\n");
            sendStatusCode(sockfd, STATUS_CRED);
            continue;
        } else {
            INFO("Username found\n");
            sendStatusCode(sockfd, STATUS_OK);
        }

        if ((res = recvCmd(sockfd)) < 0) {
            ERROR("%s\n", errString);
            return;
        }
        if (res == 0) {
            INFO("Connection closed by client\n");
            return;
        }

        if (argc < 2 || strcmp(argv[0], "pass") != 0) {
            INFO("Password authentication req. \n");
            sendStatusCode(sockfd, STATUS_CMD);
            continue;
        }

        if (strcmp(pass, argv[1]) != 0) {
            INFO("Incorrect password\n");
            sendStatusCode(sockfd, STATUS_CRED);
            continue;
        } else {
            INFO("Password correct\n");
            sendStatusCode(sockfd, STATUS_OK);
            flogged = 1;
        }
    }
    INFO("Client logged in\n");

    while (1) {
        int res;
        if ((res = recvCmd(sockfd)) < 0) {
            ERROR("%s\n", strerror(errno));
            sendStatusCode(sockfd, STATUS_QUIT);
            return;
        }
        if (res == 0) {
            INFO("Connection closed by client\n");
            return;
        }
        if (strcmp(argv[0], "cd") == 0) {  // change directory
            if (chdir(argv[1]) < 0) {
                ERROR("Bad change dir: %s\n", strerror(errno));
                sendStatusCode(sockfd, STATUS_CRED);
            } else {
                sendStatusCode(sockfd, STATUS_OK);
                getcwd(buff, 1024);
                INFO("Changed directory to %s\n", buff);
            }
        } else if (strcmp(argv[0], "dir") == 0) {  // list directory content
            DIR* d;
            struct dirent* dir;
            d = opendir(".");
            if (d) {
                // sendStatusCode(sockfd, STATUS_OK);
                while ((dir = readdir(d)) != NULL) {
                    int len = strlen(dir->d_name);
                    strcpy(buff, dir->d_name);
                    if (dir->d_type == DT_DIR)
                        buff[len++] = '/';
                    buff[len++] = '\0';
                    INFO("Sending: %s\n", buff);
                    send(sockfd, buff, len, 0);
                }
                INFO("Sent all files\n");
                closedir(d);
                send(sockfd, "\0", 1, 0);  // null terminate
            } else {
                //
            }
        } else if (strcmp(argv[0], "get") == 0) {  // get remote file
            FILE* fp;
            if ((fp = fopen(argv[1], "r")) == NULL) {
                ERROR("Cannot open file: %s\n", strerror(errno));
                sendStatusCode(sockfd, STATUS_CRED);
            } else {
                sendStatusCode(sockfd, STATUS_OK);
                send_file(sockfd, fp);
            }
            fclose(fp);
        } else if (strcmp(argv[0], "put") == 0) {
            FILE* fp = fopen(argv[2], "w");
            if (fp == NULL) {
                ERROR("Can't open %s for writing: %s\n", argv[2], strerror(errno));
                sendStatusCode(sockfd, STATUS_CRED);
            } else {
                sendStatusCode(sockfd, STATUS_OK);
                receive_file(sockfd, fp);
            }
            fclose(fp);
        } else {
            ERROR("Invalid command sent by client\n");
            sendStatusCode(sockfd, STATUS_CMD);
        }
    }
    return;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        ERROR("Usage: %s <port>\n", argv[0]);
        return -1;
    }
    int sockfd, newsockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        ERROR("socket error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in servaddr, cli_addr;
    socklen_t len = sizeof(cli_addr);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(atoi(argv[1]));

    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERROR("bind error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    INFO("Binded to port: %d\n", atoi(argv[1]));
    listen(sockfd, 5);
    fd_set waitfd;
    int maxfd = sockfd + 1;

    FD_ZERO(&waitfd);
    FD_SET(sockfd, &waitfd);

    while (1) {
        select(maxfd, &waitfd, NULL, NULL, NULL);
        if (FD_ISSET(sockfd, &waitfd)) {
            newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &len);
            if (fork() == 0) {
                close(sockfd);
                handleClient(newsockfd, &cli_addr);
                close(newsockfd);
                exit(EXIT_SUCCESS);
            } else {
                close(newsockfd);
            }
        }
    }
    return 0;
}