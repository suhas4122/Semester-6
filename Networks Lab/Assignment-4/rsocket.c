#include "rsocket.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define DEBUG_LEVEL 0

#define _INFO_L 1
#define _DEBUG_L 2
#define _ERROR_L 3

#define _COLOR_RED "1;31"
#define _COLOR_BLUE "1;34"
#define _COLOR_GREEN "0;32"

#define __LOG_COLOR(FD, CLR, CTX, TXT, args...) fprintf(FD, "\033[%sm[%s] \033[0m" TXT, CLR, CTX, ##args)
#define INFO(TXT, args...)                                                                  \
    do {                                                                                    \
        if (DEBUG_LEVEL >= _INFO_L) __LOG_COLOR(stdout, _COLOR_GREEN, "info", TXT, ##args); \
    } while (0)
#define DEBUG(TXT, args...)                                                                  \
    do {                                                                                     \
        if (DEBUG_LEVEL >= _DEBUG_L) __LOG_COLOR(stdout, _COLOR_BLUE, "debug", TXT, ##args); \
    } while (0)
#define ERROR(TXT, args...)                                                                 \
    do {                                                                                    \
        if (DEBUG_LEVEL >= _ERROR_L) __LOG_COLOR(stderr, _COLOR_RED, "error", TXT, ##args); \
    } while (0)

// ACK MESSAGE:
#define UNACKED 0
#define ACKED 1

int dropMessage(float p) {
    float sample = (float)rand() / (float)RAND_MAX;
    if (sample < p)
        return 1;
    return 0;
}

struct unack_message {
    void *data;
    time_t timestamp;
    int seq_num;
    struct sockaddr_in addr;
    int len;
    int sent_count;
    short status;
};

// simple c-array with a mutex
struct unack_mtable {
    struct unack_message msgs[_WINDOW_SIZE];
    int count;
    pthread_mutex_t mutex;
} * unack_mtable;

void init_unack_mtable(struct unack_mtable *table) {
    table->count = 0;
    for (int i = 0; i < _WINDOW_SIZE; i++) {
        table->msgs[i].data = NULL;
        memset(&table->msgs[i].addr, 0, sizeof(struct sockaddr_in));
        table->msgs[i].len = 0;
        table->msgs[i].status = ACKED;
    }
    pthread_mutex_init(&table->mutex, NULL);
}

struct recv_message {
    void *data;
    int len;
    struct sockaddr_in addr;
};

struct recv_mtable {
    struct recv_message msgs[_WINDOW_SIZE];
    int count;
    int in, out;  // circular buffer
    pthread_mutex_t mutex;
} * recv_mtable;

void init_recv_mtable(struct recv_mtable *mtable) {
    mtable->in = 0;
    mtable->out = 0;
    mtable->count = 0;
    for (int i = 0; i < _WINDOW_SIZE; i++) {
        mtable->msgs[i].data = NULL;
        mtable->msgs[i].len = 0;
        memset(&mtable->msgs[i].addr, 0, sizeof(struct sockaddr_in));
    }
    pthread_mutex_init(&mtable->mutex, NULL);  // initialize mutex
}

// add a message to the unacknowledged message table
int add_unack_msg(const void *data, ssize_t len, struct sockaddr_in *addr) {
    DEBUG("Adding new message: %.*s\n", (int)len, (char *)data);
    int idx = -1;
    pthread_mutex_lock(&unack_mtable->mutex);
    if (unack_mtable->count < _WINDOW_SIZE)
        for (int i = 0; i < _WINDOW_SIZE; i++) {
            if (unack_mtable->msgs[i].status == ACKED) {
                DEBUG("Found an empty slot at %d\n", i);
                unack_mtable->msgs[i].status = UNACKED;
                // free the previous data
                if (unack_mtable->msgs[i].data != NULL)
                    free(unack_mtable->msgs[i].data);
                // copy new data
                unack_mtable->msgs[i].data = malloc(len);
                memcpy(unack_mtable->msgs[i].data, data, len);
                // add timestamp
                unack_mtable->msgs[i].timestamp = time(NULL);
                unack_mtable->msgs[i].seq_num = i;
                unack_mtable->msgs[i].addr = *addr;
                unack_mtable->msgs[i].len = len;
                unack_mtable->msgs[i].sent_count = 1;
                unack_mtable->count++;
                DEBUG("Unack message: %.*s, len: %d, timestamp: %ld\n",
                      (int)len, (char *)data, (int)len, unack_mtable->msgs[i].timestamp);
                idx = i;
                break;
            }
        }
    pthread_mutex_unlock(&unack_mtable->mutex);
    return idx;
}

void free_unack_mtable(struct unack_mtable *table) {
    DEBUG("Freeing unack_mtable\n");
    for (int i = 0; i < _WINDOW_SIZE; i++) {
        if (table->msgs[i].data != NULL) {
            free(table->msgs[i].data);
            table->msgs[i].data = NULL;
        }
    }
    pthread_mutex_destroy(&table->mutex);
    free(table);
}

void free_recv_mtable(struct recv_mtable *table) {
    DEBUG("Freeing recv_mtable\n");
    for (int i = 0; i < _WINDOW_SIZE; i++) {
        if (table->msgs[i].data != NULL) {
            free(table->msgs[i].data);
            table->msgs[i].data = NULL;
        }
    }
    pthread_mutex_destroy(&table->mutex);
    free(table);
}

void *handle_S(void *param) {
    int sockfd = *(int *)param;
    while (1) {
        sleep(T);
        DEBUG("Checking for unacked messages\n");
        for (int i = 0; i < _WINDOW_SIZE; i++) {
            pthread_mutex_lock(&unack_mtable->mutex);
            if (unack_mtable->msgs[i].status == ACKED) {
                pthread_mutex_unlock(&unack_mtable->mutex);
                continue;
            }
            time_t curr = time(NULL);
            if (curr - unack_mtable->msgs[i].timestamp > 2 * T) {
                INFO("Found unacked message %.*s at index %d, resending ...\n",
                     (int)unack_mtable->msgs[i].len, (char *)unack_mtable->msgs[i].data, i);
                char *data = unack_mtable->msgs[i].data;
                int len = unack_mtable->msgs[i].len;
                void *nbuf = malloc(sizeof(int) + len);
                memcpy(nbuf, &unack_mtable->msgs[i].seq_num, sizeof(int));
                memcpy(nbuf + sizeof(int), data, len);
                ssize_t n = sendto(sockfd, nbuf, sizeof(int) + len, 0,
                                   (struct sockaddr *)&unack_mtable->msgs[i].addr,
                                   sizeof(struct sockaddr_in));
                unack_mtable->msgs[i].sent_count++;
                if (n < 0) {
                    perror("sendto");
                    exit(1);
                }
                free(nbuf);
                unack_mtable->msgs[i].timestamp = time(NULL);
                // DEBUG("Resent message at index %d\n", i);
            }
            pthread_mutex_unlock(&unack_mtable->mutex);
        }
    }
}

int add_recv_msg(struct recv_mtable *table, const void *data, ssize_t len,
                 struct sockaddr_in addr) {
    DEBUG("Adding message to recv_mtable\n");
    DEBUG("Message: %s\n", data);
    int idx = -1;
    pthread_mutex_lock(&table->mutex);
    if (table->count < _WINDOW_SIZE) {
        if (table->msgs[table->in].data != NULL) {
            free(table->msgs[table->in].data);
        }
        table->msgs[table->in].data = (char *)malloc(len);
        memcpy(table->msgs[table->in].data, data, len);
        table->msgs[table->in].len = len;
        table->msgs[table->in].addr = addr;
        idx = table->in;
        table->in = (table->in + 1) % _WINDOW_SIZE;
        table->count++;
        DEBUG("Added message to recv_mtable at index %d\n", idx);
    }
    pthread_mutex_unlock(&table->mutex);
    return idx;
}

int min(size_t a, size_t b) { return a < b ? a : b; }

int get_recv_msg(struct recv_mtable *table, char *data, ssize_t len, struct sockaddr_in *addr) {
    // DEBUG("Getting message from recv_mtable\n");
    // DEBUG("Max message len: %d\n", (int)len);
    int ret = -1;
    pthread_mutex_lock(&table->mutex);
    if (table->count > 0) {
        DEBUG("Message: %s\n", (char *)table->msgs[table->out].data);
        ret = min(len, table->msgs[table->out].len);
        memcpy(data, table->msgs[table->out].data, ret);
        *addr = table->msgs[table->out].addr;
        free(table->msgs[table->out].data);
        table->msgs[table->out].data = NULL;  // set to NULL to avoid double free
        table->out = (table->out + 1) % _WINDOW_SIZE;
        table->count--;
    }
    pthread_mutex_unlock(&table->mutex);
    return ret;
}

pthread_t R, S;
#define BUFF_SIZE 1024

void *handle_R(void *param) {
    INFO("Starting receiver thread\n");
    INFO("p: %f", P);
    srand(time(NULL));
    int sockfd = *(int *)param;
    char buff[BUFF_SIZE];
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int total_times = 0;
    while (1) {
        ssize_t n = recvfrom(sockfd, buff, BUFF_SIZE, 0, (struct sockaddr *)&addr, &addrlen);
        DEBUG("Received message from %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        DEBUG("Message: %s, len: %d\n", buff, (int)n);
        if (n < 0) {
            perror("recvfrom");
            pthread_exit(NULL);
        }
        int drop = dropMessage(P);
        if (drop) continue;
        int seq_num = *(int *)buff;
        // this is a acknowledgement
        if (n == sizeof(int)) {  // ack
            pthread_mutex_lock(&unack_mtable->mutex);
            if (unack_mtable->msgs[seq_num].status == UNACKED) {
                unack_mtable->msgs[seq_num].status = ACKED;
                unack_mtable->count--;
                total_times += unack_mtable->msgs[seq_num].sent_count;
                if (unack_mtable->count == 0) {
#ifndef NDEBUG
                    FILE *fp = fopen("result.txt", "a");
                    fprintf(fp, "%lf, %d\n", P, total_times);
                    fclose(fp);
                    exit(0);
#endif
                    total_times = 0;
                }
            }
            pthread_mutex_unlock(&unack_mtable->mutex);
        } else {  // data
            char *data = (char *)malloc(n - sizeof(int));
            memcpy(data, buff + sizeof(int), n - sizeof(int));
            add_recv_msg(recv_mtable, data, n - sizeof(int), addr);
            // send ack of seq_num
            int ack = seq_num;
            n = sendto(sockfd, &ack, sizeof(int), 0, (struct sockaddr *)&addr, addrlen);
        }
    }
}

int r_socket(int __domain, int __type, int __protocol) {
    if (__type != SOCK_MRP)
        return -1;
    int _socket = socket(__domain, SOCK_DGRAM, __protocol);
    if (_socket < 0)
        return _socket;

    recv_mtable = (struct recv_mtable *)malloc(sizeof(struct recv_mtable));
    init_recv_mtable(recv_mtable);

    unack_mtable = (struct unack_mtable *)malloc(sizeof(struct unack_mtable));
    init_unack_mtable(unack_mtable);

    int *param = (int *)malloc(sizeof(int));
    *param = _socket;
    pthread_create(&R, NULL, handle_R, param);
    pthread_create(&S, NULL, handle_S, param);

    return _socket;
}

int r_bind(int __fd, const struct sockaddr *__addr, socklen_t __len) {
    return bind(__fd, __addr, __len);
}

ssize_t r_sendto(int __fd, const void *__buf, size_t __n,
                 int __flags, const struct sockaddr *__addr,
                 socklen_t __addr_len) {
    if (__fd < 0)
        return -1;
    DEBUG("Sending message %.*s to %s:%d\n", (int)__n, (char *)__buf,
          inet_ntoa(((struct sockaddr_in *)__addr)->sin_addr),
          ntohs(((struct sockaddr_in *)__addr)->sin_port));
    int ret = add_unack_msg(__buf, __n, (struct sockaddr_in *)__addr);
    while (ret < 0) {
        ret = add_unack_msg(__buf, __n, (struct sockaddr_in *)__addr);
        usleep(100);
    }
    DEBUG("Added message to unack_mtable at index %d\n", ret);
    void *nbuf = malloc(sizeof(int) + __n);
    memcpy(nbuf, &ret, sizeof(int));
    memcpy(nbuf + sizeof(int), __buf, __n);
    ssize_t n = sendto(__fd, nbuf, sizeof(int) + __n, __flags, __addr, __addr_len);
    free(nbuf);
    return n;
}

ssize_t r_recvfrom(int __fd, void *__restrict__ __buf,
                   size_t __n, int __flags, struct sockaddr *__restrict__ __addr,
                   socklen_t *__restrict__ __addr_len) {
    if (__fd < 0)
        return -1;
    int ret;
    while ((ret = get_recv_msg(recv_mtable, (char *)__buf, __n, (struct sockaddr_in *)__addr)) < 0) {
        // DEBUG("waiting for data\n");
        usleep(100);
    }
    return ret;
}

int r_close(int __fd) {
    if (__fd < 0)
        return -1;
    pthread_cancel(R);
    pthread_cancel(S);
    pthread_join(R, NULL);
    pthread_join(S, NULL);
    free_recv_mtable(recv_mtable);
    free_unack_mtable(unack_mtable);
    return close(__fd);
}