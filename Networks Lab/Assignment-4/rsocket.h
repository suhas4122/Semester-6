#ifndef MRP_SOCKET_H
#define MRP_SOCKET_H

#include <sys/socket.h>

#ifndef _DEBUG
#define NDEBUG
#endif

#define T 2
#ifndef P
#define P 0.05
#endif
#define SOCK_MRP 42
#define _WINDOW_SIZE 0x3F

int r_socket(int __domain, int __type, int __protocol);
int r_bind(int __fd, const struct sockaddr *__addr, socklen_t __len);
ssize_t r_sendto(int __fd, const void *__buf, size_t __n,
                 int __flags, const struct sockaddr *__addr, socklen_t __addr_len);
ssize_t r_recvfrom(int __fd, void *__restrict__ __buf,
                   size_t __n, int __flags, struct sockaddr *__restrict__ __addr,
                   socklen_t *__restrict__ __addr_len);
int r_close(int __fd);

#endif  // MRP_SOCKET_H