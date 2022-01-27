#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>

#define BUFFER_SIZE 100                     // Size of the buffer to read from recvfrom() calls
#define PORT_NO 20200                       // Port number to connect to

int main() {
    int sockfd;                                             // Socket file descriptor
    struct sockaddr_in servaddr, cliaddr;                   // Socket address structures
    char buffer[BUFFER_SIZE];                               // Buffer to read from recvfrom() calls

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);                // Create a socket
    if (sockfd < 0) {                                                                       
        perror("Unable to create socket");                  // Error handling
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));                 // Clear the server socket address structure
    memset(&cliaddr, 0, sizeof(cliaddr));                   // Clear the client socket address structure

    servaddr.sin_family = AF_INET;                          // Set family to Internet
    servaddr.sin_port = htons(PORT_NO);                     // Set port number
    servaddr.sin_addr.s_addr = INADDR_ANY;                  // Set IP address to localhost

    if(bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {      // Bind the socket to the address
        perror("Unable to bind");                                                     // Error handling
        exit(1);
    }

    socklen_t len = sizeof(cliaddr);                        // Length of the client address structure
    printf("Server Running....\n\n");
                                                               
    while(1) {
        for(int i = 0; i < BUFFER_SIZE; i++){
            buffer[i] = '\0';
        }
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&cliaddr, &len);       // Receive from the client
        if(n < 0) {
            perror("Unable to read from socket\n");         // Error handling
        }
        printf("Data recieved from UDP client\n\n");
        printf("DNS name: %s\n\n", buffer);

        struct hostent *he;                                 // Structure to store host information
        struct in_addr **addr_list;                         // List of IP addresses
        struct in_addr addr;                                // IP address structure
        he = gethostbyname(buffer);                         // Get the host information
        if (he == NULL) {                                   // If the host information is not found
            sendto(sockfd, "0.0.0.0", strlen("0.0.0.0"), 0, (const struct sockaddr *) &cliaddr, sizeof(cliaddr));
            herror("gethostbyname"); 
            printf("\n");
        }else{
            addr_list = (struct in_addr **)he->h_addr_list;     // Get the list of IP addresses
            for(int i = 0; addr_list[i] != NULL; i++) {         // Print all the IP addresses
                char* ip = inet_ntoa(*addr_list[i]);
                sendto(sockfd, ip, strlen(ip), 0, (const struct sockaddr *) &cliaddr, sizeof(cliaddr));   // Send the IP address to the client
            }
            printf("DNS addresses sent to UDP client\n\n");
        }
    }

    close(sockfd);                                                  // Close the socket
    return 0;    
}