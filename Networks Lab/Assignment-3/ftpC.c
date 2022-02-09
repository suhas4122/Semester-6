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

const char* sprompt = "myFTP> ";
char* errString = "";
// ftp prompt
char cmd[1024];
// cmd length
int cmdLen = 0;
char* argv[1024];
int argc = 0;
// Flag if client is connected to server
int fconnected = 0;
// flag if client is logged in
int flogged = 0;
char status[10];

/**
 * @brief Prints the prompt and takes input from the user   
 * 
 */
void prompt() {
    printf("%s", sprompt);
    scanf("%[^\n]s", cmd);
    getchar();  // eat newline
    cmdLen = strlen(cmd);
    cmd[cmdLen] = '\0';
    cmdLen++;
    char* cmdCpy = malloc(cmdLen);
    strcpy(cmdCpy, cmd);
    tokenize(cmdCpy);
}

/**
 * @brief Updates the Status code of last cmd and sets error string based on last exec. command and status
 * 
 * @param sockfd socket to recieve statuc code from
 * @return int 1 if successfull, -1 if error, 0 if connection closed
 */
int updateStat(int sockfd) {
    int n = 0;
    char prv = ' ';
    while (prv != '\0') {
        int r = recv(sockfd, status + n, 10 - n, 0);
        if (r < 0 && errno != EINTR) {
            errString = strerror(errno);
            return -1;
        }
        if (r == 0) {
            errString = "Connection closed";
            return 0;
        }
        n += r;
        prv = status[n - 1];
    }

    if (strcmp(status, STATUS_OK) == 0)
        errString = "OK";
    else if (strcmp(status, STATUS_BAD) == 0) {
        errString = "Remote server went bad";
    } else if (strcmp(status, STATUS_CMD) == 0) {
        errString = "login required before issuing commands";
    } else if (strcmp(argv[0], "cd") == 0) {
        errString = "no such file or directory";
    } else if (strcmp(argv[0], "get") == 0) {
        errString = "Remote file not found";
    } else if (strcmp(argv[0], "put") == 0) {
        errString = "Unable to open remote file";
    } else if (strcmp(argv[0], "user") == 0) {
        errString = "Username not found";
    } else if (strcmp(argv[0], "pass") == 0) {
        errString = "Incorrect password";
    } else if (strcmp(argv[0], "quit") == 0) {
        errString = "Quit";
    } else {
        errString = "Unknown command";
    }
    DEBUG("Status code: %s\n", status);
    return 1;
}

char* IP;
int PORT;

/**
 * @brief Function to establish initial connection with the server
 * 
 * @param sockfd socket to make connection with
 * @param serveaddr struct that stores server IP and port
 * @return int 1 if successfull, 0 if error
 */
int Connect(int sockfd, struct sockaddr_in* serveaddr) {
    if (fconnected)
        return 1;
    prompt();
    if (argc < 3 || strcmp("open", argv[0]) != 0) {
        INFO("Usage: open <IP> <PORT>\n");
        return 0;
    }
    IP = argv[1];
    int res = inet_pton(AF_INET, IP, &serveaddr->sin_addr);
    if (res == 0) {
        ERROR("Invalid ipv4 address format\n");
        return 0;
    }
    errno = 0;
    char* tmp;
    PORT = strtol(argv[2], &tmp, 10);
    // todo: make error more specific acc. to case
    if (errno != 0 || *tmp != '\0' || PORT < 20000 || PORT > 65535) {
        ERROR("Invalid port number: %s\n", argv[2]);
        return 0;
    }
    serveaddr->sin_port = htons(PORT);
    if (connect(sockfd, (struct sockaddr*)serveaddr, sizeof(*serveaddr)) < 0) {
        ERROR("Connection failed\n");
        return 0;
    } else {
        fconnected = 1;
        return 1;
    }
}

/**
 * @brief Function to assist sending login id and password to the server
 * 
 * @param sockfd socket to send login id and password to
 * @return int 1 if successfull, 0 if error
 */
int Authenticate(int sockfd) {
    if (flogged)
        return 1;
    prompt();
    if (argc < 2 || strcmp("user", argv[0]) != 0) {
        INFO("Usage: user <username>\n");
        return 0;
    }
    if ((send(sockfd, cmd, cmdLen, 0)) < 0) {
        ERROR("Send failed: %s\n", strerror(errno));
        return 0;
    }
    updateStat(sockfd);
    if (strcmp(status, STATUS_OK) != 0) {
        ERROR("%s\n", errString);
        return 0;
    } else
        INFO("Username found\n");

    prompt();
    if (argc < 2 || strcmp("pass", argv[0]) != 0) {
        INFO("Usage: pass <password>\n");
        return 0;
    }
    if (send(sockfd, cmd, cmdLen, 0) < 0) {
        ERROR("Send failed: %s\n", strerror(errno));
        return 0;
    }
    updateStat(sockfd);
    if (strcmp(status, STATUS_OK) != 0) {
        ERROR("%s\n", errString);
        return 0;
    } else {
        DEBUG("Password matched\n");
        flogged = 1;
    }
    return 1;
}

int main() {
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        ERROR("Socket creation failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serveaddr;
    memset(&serveaddr, 0, sizeof(serveaddr));
    serveaddr.sin_family = AF_INET;

    while (1) {
        while (!fconnected) {
            flogged = 0;
            if (Connect(sockfd, &serveaddr) == 1)
                INFO("Connected to %s:%d\n", IP, (int)PORT);
        }
        while (!flogged) {
            if (Authenticate(sockfd))
                INFO("Logged in\n");
        }
        prompt();
        if (argc < 1) continue;             // skip empty
        if (strcmp("lcd", argv[0]) == 0) {  // change local directory
            if (argc < 2)
                INFO("Usage: lcd <path>\n");
            else if (chdir(argv[1]) < 0)
                ERROR("%s\n", strerror(errno));
        } else if (strcmp("cd", argv[0]) == 0) {  // change remote directory
            if (argc < 2) {
                INFO("Usage: cd <path>\n");
                continue;
            }
            int res;
            if ((res = send(sockfd, cmd, cmdLen, 0)) < 0) {
                ERROR("%s\n", strerror(errno));
                continue;
            }
            if (res == 0) {
                ERROR("Connection closed\n");
                fconnected = 0;
                continue;
            }
            updateStat(sockfd);
            if (strcmp(status, STATUS_OK) != 0)
                ERROR("%s\n", errString);
        } else if (strcmp("dir", argv[0]) == 0) {  // list remote directory contents
            if (argc != 1) {
                INFO("Usage: dir\n");
                continue;
            }
            int res;
            if ((res = send(sockfd, cmd, cmdLen, 0)) < 0) {
                ERROR("%s\n", strerror(errno));
                continue;
            }
            if (res == 0) {
                ERROR("Connection closed\n");
                fconnected = 0;
                continue;
            }
            // updateStat(sockfd);  // todo: Is status code needed here ?
            // if (strcmp(status, STATUS_OK) != 0) {
            //     ERROR("%s\n", errString);
            //     continue;
            // }
            char buffer[100];
            int flag = 1;
            char prev = ' ';
            while (flag) {
                for (int i = 0; i < 100; i++) {
                    buffer[i] = '\0';
                }
                int n = recv(sockfd, buffer, 100, 0);
                if (n < 0) {
                    ERROR("%s\n", strerror(errno));
                    continue;
                } else {
                    for (int i = 0; i < n; i++) {
                        if (buffer[i] == '\0' && prev == '\0') {
                            printf("\n");
                            flag = 0;
                            break;
                        } else if (buffer[i] == '\0') {
                            printf("\n");
                        } else {
                            printf("%c", buffer[i]);
                        }
                        prev = buffer[i];
                    }
                }
            }
        } else if (strcmp("get", argv[0]) == 0) {  // get remote file
            if (argc < 3) {
                INFO("Usage: get <remote_file> <local_file>\n");
                continue;
            }
            int res;
            if ((res = send(sockfd, cmd, cmdLen, 0)) < 0) {
                ERROR("%s\n", strerror(errno));
                continue;
            }
            if (res == 0) {
                ERROR("Connection closed\n");
                fconnected = 0;
                continue;
            }
            updateStat(sockfd);
            if (strcmp(status, STATUS_OK) != 0) {
                ERROR("get: %s\n", errString);
                continue;
            }
            FILE* fp = fopen(argv[2], "w");
            receive_file(sockfd, fp);
            fclose(fp);
        } else if (strcmp("put", argv[0]) == 0) {  // put local file to remote
            if (argc < 3) {
                INFO("Usage: put <local_file> <remote_file>\n");
                continue;
            }
            FILE* fp = fopen(argv[1], "r");
            if (fp == NULL) {
                INFO("Cannot open file %s\n", argv[1]);
                continue;
            }
            int res;
            if ((res = send(sockfd, cmd, cmdLen, 0)) < 0) {
                ERROR("%s\n", strerror(errno));
                continue;
            }
            if (res == 0) {
                ERROR("Connection closed\n");
                fconnected = 0;
                continue;
            }
            updateStat(sockfd);
            if (strcmp(status, STATUS_OK) != 0) {
                ERROR("put: %s\n", errString);
                continue;
            }
            send_file(sockfd, fp);
            fclose(fp);
        } else if (strcmp("mget", argv[0]) == 0) {  // get multiple remote files
            int flag = 1;
            for (int i = 1; i < argc && flag; i++) {
                if(i != argc -1)
                    argv[i][strlen(argv[i])-1] = '\0';
                sprintf(cmd, "get %s %s", argv[i], argv[i]);
                cmdLen = strlen(cmd);
                cmd[cmdLen++] = '\0';
                int res = send(sockfd, cmd, cmdLen, 0);
                if (res < 0) {
                    ERROR("%s\n", strerror(errno));
                    flag = 0;
                    break;
                }
                if (res == 0) {
                    ERROR("Connection closed\n");
                    fconnected = 0;
                    flag = 0;
                    break;
                }
                updateStat(sockfd);
                if (strcmp(status, STATUS_OK) != 0) {
                    ERROR("mget: Could not get %s\n", argv[i]);
                    flag = 0;
                    break;
                }
                FILE* fp = fopen(argv[i], "w");
                receive_file(sockfd, fp);
                fclose(fp);
            }
        } else if (strcmp("mput", argv[0]) == 0) {  // put local files to remote
            int flag = 1;
            for (int i = 1; i < argc && flag; i++) {
                if(i != argc -1)
                    argv[i][strlen(argv[i])-1] = '\0';
                FILE* fp = fopen(argv[i], "r");
                if (fp == NULL) {
                    INFO("Cannot open file %s\n", argv[i]);
                    flag = 0;
                    break;
                }
                sprintf(cmd, "put %s %s", argv[i], argv[i]);
                cmdLen = strlen(cmd);
                cmd[cmdLen++] = '\0';
                int res = send(sockfd, cmd, cmdLen, 0);
                if (res < 0) {
                    ERROR("%s\n", strerror(errno));
                    flag = 0;
                    break;
                }
                if (res == 0) {
                    ERROR("Connection closed\n");
                    fconnected = 0;
                    flag = 0;
                    break;
                }
                updateStat(sockfd);
                if (strcmp(status, STATUS_OK) != 0) {
                    ERROR("mput: Could not put %s\n", argv[i]);
                    flag = 0;
                    break;
                }
                send_file(sockfd, fp);
                fclose(fp);
            }
        } else if (strcmp("quit", argv[0]) == 0) {
            if (argc != 1) {
                INFO("Usage: quit\n");
                continue;
            }
            close(sockfd);
            exit(EXIT_SUCCESS);
        } else {
            ERROR("Unknown command\n");
        }
    }
}