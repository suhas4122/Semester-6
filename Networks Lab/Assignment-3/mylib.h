#include <stdio.h>

// https://github.com/kuroidoruido/ColorLog/blob/master/colorlog.h
#define _COLOR_RED "1;31"
#define _COLOR_BLUE "1;34"
#define _COLOR_GREEN "0;32"

extern FILE* _logFp;
void initLogger(const char*);
void log_print(FILE*, const char*, ...);

#define __LOG_COLOR(FD, CLR, CTX, TXT, args...) log_print(FD, "\033[%sm[%s] \033[0m" TXT, CLR, CTX, ##args)
#define INFO(TXT, args...) __LOG_COLOR(stdout, _COLOR_GREEN, "info", TXT, ##args)
#define DEBUG(TXT, args...) __LOG_COLOR(stdout, _COLOR_BLUE, "debug", TXT, ##args)
#define ERROR(TXT, args...) __LOG_COLOR(stderr, _COLOR_RED, "error", TXT, ##args)

/**
 * STATUS_CODE:
 * 200: OK
 * 500: Command in wrong order
 * 600: Invalid credentials
 * 400: Server went bad
 */
#define STATUS_OK "200"
#define STATUS_CMD "600"
#define STATUS_CRED "500"
#define STATUS_BAD "400"
#define STATUS_QUIT "700"

#define BUFFER_SIZE 100
struct data_packet {
    char pos;
    short len;
    char* data;
};

// argument buffer
extern char* argv[1024];
// number of arguments
extern int argc;

void serialize(unsigned char* buff, struct data_packet* pckt);
void tokenize(char* cmdCpy);
int send_file(int sockfd, FILE* fp);
int receive_file(int sockfd, FILE* fp);