#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]){
    int i;
    struct hostent *he;
    struct in_addr **addr_list;
    struct in_addr addr;
    
    he = gethostbyname("www.yahoo.com");
    if (he == NULL) { 
        herror("gethostbyname"); 
        exit(1);
    }

    printf("IP address: %s\n", inet_ntoa(*(struct in_addr*)he->h_addr));
    printf("All addresses: \n");

    addr_list = (struct in_addr **)he->h_addr_list;
    for(i = 0; addr_list[i] != NULL; i++) {
        printf("%s\n", inet_ntoa(*addr_list[i]));
    }
    printf("\n");

    return 0;
}