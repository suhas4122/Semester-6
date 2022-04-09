#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define DEBUG_LEVEL 4

#define _INFO_L 1
#define _DEBUG_L 2
#define _ERROR_L 3

#define _COLOR_RED "1;31"
#define _COLOR_BLUE "1;34"
#define _COLOR_GREEN "0;32"

#define __LOG_COLOR(FD, CLR, CTX, TXT, args...) fprintf(FD, "\033[%sm[%s]: \033[0m" TXT, CLR, CTX, ##args)
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

#define LOG(HEADER, COLOR, args...)                                             \
    do {                                                                        \
        if (DEBUG_LEVEL >= _INFO_L) __LOG_COLOR(stdout, COLOR, HEADER, ##args); \
    } while (0)

#define MAX_TTL 16
#define MAX_PROBES 3
#define PORT_NO 32164
#define PAYLOAD_SIZE 52
#define MY_PORT 20000

double time_diff(struct timeval *send, struct timeval *recv) {
    struct timeval diff;
    timersub(recv, send, &diff);
    return diff.tv_sec * 1000 + diff.tv_usec / 1000;
}

void prettyprint(int ttl, char *addr, int difftime) {
    if (ttl == 1) fprintf(stdout, "\033[%smHop_Count(TTL)\tIP_Address\tResponse_Time\033[0m\n", _COLOR_GREEN);
    if (addr == NULL) fprintf(stdout, "\t%-4d\t%-8s\t%-6s\n", ttl, "     *", "     *");
    else printf("\t%-4d\t%-8s\t%6d ms\n", ttl, addr, difftime);
}

static u_int16_t checksum_ip(u_int16_t *addr, u_int32_t count) {
    register u_int64_t sum = 0;
    for (; count > 1; count -= 2) sum += *addr++;
    if (count > 0) sum += ((*addr) & htons(65280));
    /* fold sum to 16 bits and add carrier to result */
    for (; (sum >> 16); sum = (sum & 65535) + (sum >> 16))
        ;
    // one's complement
    sum = ~sum;
    return ((u_int16_t)sum);
}

void checksum_udp(struct iphdr *pIph, u_int16_t *ipPayload) {
    register u_int64_t sum = 0;
    struct udphdr *udphdrp = (struct udphdr *)(ipPayload);
    u_int16_t udpLen = htons(udphdrp->len);
    sum += (pIph->saddr >> 16) & 65535;
    sum += (pIph->saddr) & 65535;
    sum += (pIph->daddr >> 16) & 65535;
    sum += (pIph->daddr) & 65535;
    sum += htons(IPPROTO_UDP);
    sum += udphdrp->len;

    udphdrp->check = 0;
    for (; udpLen > 1; udpLen -= 2) sum += *ipPayload++;
    if (udpLen > 0) sum += ((*ipPayload) & htons(0xFF00));
    for (; (sum >> 16); sum = (sum & 65535) + (sum >> 16))
        ;
    // printf("one's complementn");
    sum = ~sum;
    udphdrp->check = ((u_int16_t)sum == 0x0000) ? 0xFFFF : (u_int16_t)sum;
}

int main(const int argc, char *argv[]) {
    srand(time(NULL));
    if (argc < 2) {
        ERROR("PEnter domain name as an argument\n");
        exit(1);
    }
    struct hostent *he = gethostbyname(argv[1]);
    if (he == NULL) {
        ERROR("DNS couldn't be resolved\n");
        exit(1);
    }
    struct in_addr **addr_list;
    addr_list = (struct in_addr **)he->h_addr_list;
    if (he == NULL || *addr_list == NULL) {
        ERROR("DNS couldn't be resolved\n");
        exit(1);
    }

    struct in_addr *ip_address = addr_list[0];
    INFO("IP: %s\n", inet_ntoa(*ip_address));

    struct sockaddr_in sin, din;
    memset(&sin, 0, sizeof(sin));
    memset(&din, 0, sizeof(din));

    sin.sin_family = AF_INET;
    sin.sin_port = htons(MY_PORT);
    sin.sin_addr.s_addr = INADDR_ANY;

    din.sin_family = AF_INET;
    din.sin_port = htons(PORT_NO);
    din.sin_addr = *ip_address;

    int sockfd_udp, sock_icmp;

    if ((sockfd_udp = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) < 0) {
        ERROR("Not able to create UDP raw socket: %s", strerror(errno));
        exit(1);
    }
    // set IP_HDRINCL to true to tell the kernel that headers are included in the packet
    const int opt = 1;
    if (setsockopt(sockfd_udp, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt)) < 0) {
        ERROR("Cannot set UDP raw socket option: %s", strerror(errno));
        exit(1);
    }

    if (bind(sockfd_udp, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        ERROR("Cannot bind UDP raw socket: %s", strerror(errno));
        exit(1);
    }

    if ((sock_icmp = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
        ERROR("Not able to create ICMP raw socket: %s", strerror(errno));
        exit(1);
    }

    if (bind(sock_icmp, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        ERROR("Cannot bind ICMP raw socket: %s", strerror(errno));
        exit(1);
    }

    char buff[1024];
    for (int ttl = 1; ttl <= MAX_TTL; ttl++) {
        int tout = 0;
        int done = 0;
        for (int pr = 0; pr < MAX_PROBES; pr++) {
            memset(buff, 0, sizeof(buff));
            struct iphdr *ip = (struct iphdr *)buff;
            struct udphdr *udp = (struct udphdr *)(buff + sizeof(struct iphdr));
            char *udp_payload = (char *)(buff + sizeof(struct iphdr) + sizeof(struct udphdr));

            for (int i = 0; i < PAYLOAD_SIZE; i++) {    
                udp_payload[i] = (char)(rand() % 256);  // fill the payload with random data
            }
            // Fill the UDP header
            udp->dest = din.sin_port;
            udp->source = sin.sin_port;
            udp->len = htons((uint16_t)(sizeof(struct udphdr) + PAYLOAD_SIZE));
            udp->check = 0;
            // checksum_udp(ip, (u_int16_t *)(buff + sizeof(struct iphdr)));

            // Fill the IP header
            ip->daddr = din.sin_addr.s_addr;
            ip->frag_off = (uint16_t)0;
            ip->id = htons((uint16_t)30033);
            ip->ihl = 5;
            ip->protocol = IPPROTO_UDP;
            ip->saddr = sin.sin_addr.s_addr;
            ip->tos = 0;
            ip->tot_len = htons((uint16_t)(sizeof(struct iphdr) + sizeof(struct udphdr) + PAYLOAD_SIZE));
            ip->ttl = ttl;
            ip->version = 4;
            ip->check = checksum_ip((u_int16_t *)ip, sizeof(struct iphdr));

            struct timeval start, stop, curr;
            gettimeofday(&start, NULL);
            int stat = sendto(sockfd_udp, buff, sizeof(struct iphdr) + sizeof(struct udphdr) + PAYLOAD_SIZE, 0, (const struct sockaddr *)&din, sizeof(din));
            if (stat < 0) {
                ERROR("Not able to send UDP packet: %s", strerror(errno));
                close(sockfd_udp);
                close(sock_icmp);
                exit(1);
            }
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 1e6;
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(sock_icmp, &fds);
            while (1) {
                FD_ZERO(&fds);
                FD_SET(sock_icmp, &fds);
                int ret = select(sock_icmp + 1, &fds, NULL, NULL, &tv);
                if (ret < 0) {
                    ERROR("Select: %s", strerror(errno));
                    close(sock_icmp);
                    close(sockfd_udp);
                    exit(0);
                } else if (ret == 0) {
                    tout = 1;
                    break;
                } else if (FD_ISSET(sock_icmp, &fds)) {
                    char recvbuffer[1024];
                    memset(recvbuffer, 0, sizeof(recvbuffer));
                    struct sockaddr_in src_addr;
                    socklen_t src_addr_len = sizeof(src_addr);

                    int recvlen = recvfrom(sock_icmp, recvbuffer, 1024, 0, (struct sockaddr *)&src_addr, &src_addr_len);
                    if (recvlen < 0) {
                        ERROR("Recvfrom: %s", strerror(errno));
                        close(sockfd_udp);
                        close(sock_icmp);
                        exit(1);
                    }
                    struct iphdr *recv_ip = (struct iphdr *)recvbuffer;

                    if (recv_ip->protocol == IPPROTO_ICMP) {
                        struct icmphdr *recv_icmp = (struct icmphdr *)(recvbuffer + sizeof(struct iphdr));
                        if (recv_icmp->type == ICMP_TIME_EXCEEDED && recv_icmp->code == ICMP_EXC_TTL) {
                            gettimeofday(&stop, NULL);
                            prettyprint(ttl, inet_ntoa(src_addr.sin_addr), time_diff(&start, &stop));
                            done = 1;
                            done = 1;
                            break;
                        } else if (recv_icmp->type == ICMP_DEST_UNREACH && recv_icmp->code == ICMP_PORT_UNREACH) {
                            if (recv_ip->saddr == din.sin_addr.s_addr) {
                                gettimeofday(&stop, NULL);
                                prettyprint(ttl, inet_ntoa(src_addr.sin_addr), time_diff(&start, &stop));
                                done = 1;
                                close(sockfd_udp);
                                close(sock_icmp);
                                done = 1;
                                exit(0);
                            } else {
                                gettimeofday(&curr, NULL);
                                struct timeval diff;
                                timersub(&curr, &start, &diff);
                                int diff_time = diff.tv_sec * 1e6 + diff.tv_usec / 1000;
                                if (diff_time > 1000000) {
                                    tout = 1;
                                    break;
                                }
                                tv.tv_sec = 0;
                                tv.tv_usec = 1e6 - diff_time;
                            }
                        }
                    } else {
                        gettimeofday(&curr, NULL);
                        struct timeval diff;
                        timersub(&curr, &start, &diff);
                        int diff_time = diff.tv_sec * 1e6 + diff.tv_usec / 1000;
                        if (diff_time > 1e6) {
                            tout = 1;
                            break;
                        }
                        tv.tv_sec = 0;
                        tv.tv_usec = 1e6 - diff_time;
                    }
                }
            }
            if (done == 1)
                break;
        }
        if (tout) {
            prettyprint(ttl, NULL, -1);
        }
    }
    close(sockfd_udp);
    close(sock_icmp);
    return 0;
}