#include "mylib.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

FILE* _logFp = NULL;
void initLogger(const char* logFile) {
    _logFp = logFile ? fopen(logFile, "w") : stdout;
}

void log_print(FILE* fp, const char* fmt, ...) {
    if (_logFp != NULL)
        fp = _logFp;
    va_list args;
    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    fflush(fp);
    va_end(args);
}

// tokenize the input command
void tokenize(char* cmdCpy) {
    char* token = strtok(cmdCpy, " ");
    int i = 0;
    while (token != NULL) {
        argv[i] = token;
        token = strtok(NULL, " ");
        i++;
    }
    argc = i;
}

// assume len is already Network Byte Order
void serialize(unsigned char* buff, struct data_packet* pckt) {
    int offset = 0;
    memcpy(buff, &(pckt->pos), sizeof(char));
    offset += sizeof(char);
    int _len = htons(pckt->len);
    memcpy(buff + offset, &_len, sizeof(short));
    offset += sizeof(short);
    memcpy(buff + offset, pckt->data, sizeof(char) * pckt->len);
}

/**
 * @brief Sends content of file to the socket specified
 *
 * @param sockfd destination socket file descriptor
 * @param fp source File pointer
 * @return 0 if successfull or -1 if Error in sending or reading
 */
int send_file(int sockfd, FILE* fp) {
    char buffer[BUFFER_SIZE + 1];
    short n = 0;
    struct data_packet pckt;

    size_t pcktlen = sizeof(char) + sizeof(short);
    unsigned char spckt[pcktlen + BUFFER_SIZE + 1];
    while (1) {
        n = (short)fread(buffer, 1, BUFFER_SIZE, fp);
        if (n < BUFFER_SIZE)
            pckt.pos = 'L';
        else
            pckt.pos = 'M';
        pckt.data = buffer;
        pckt.len = n;
        serialize(spckt, &pckt);
        if (send(sockfd, spckt, pcktlen + n, 0) < 0) {
            ERROR("%s\n", strerror(errno));
            return -1;
        }
        if (pckt.pos == 'L')
            break;
    }
    return 0;
}

/**
 * @brief Receives stream of bytes from source socket
 * and writes content to the destination file
 *
 * @param sockfd Source file descriptor
 * @param fp  Destination File pointer
 * @return 0 if successfull or -1 if Error in sending or reading
 */
int receive_file(int sockfd, FILE* fp) {
    int n = 0;
    unsigned char spckt[200];
    char type = 'M';
    int len = 0;
    while (type == 'M') {
        n = recv(sockfd, spckt, 1, 0);
        if (n < 0) {
            ERROR("%s\n", strerror(errno));
            return -1;
        }
        type = spckt[0];
        n = 0;
        int left = sizeof(short);
        while (left > 0) {
            int r = recv(sockfd, spckt + n, left, 0);
            if (r < 0) {
                ERROR("%s\n", strerror(errno));
                return -1;
            }
            n += r;
            left -= r;
        }
        len = ntohs(*((short*)spckt));
        int tot = 0;
        while (tot < len) {
            int minv = len - tot;
            if (minv > sizeof(spckt))
                minv = sizeof(spckt);
            n = recv(sockfd, spckt, minv, 0);
            if (n < 0) {
                ERROR("%s\n", strerror(errno));
                return -1;
            }
            if (n == 0) {
                ERROR("Connection closed\n");
                return -1;
            }
            fwrite(spckt, sizeof(char), n, fp);
            tot += n;
        }
    }
    return 0;
}
