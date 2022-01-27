#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>

#define BUFFER_SIZE 100                             // Size of the buffer to read from the file
#define PORT_NO 20200                               // Port number to connect to

int main(int argc, char *argv[]) {
    int sockfd;                                     // Socket file descriptor
    struct sockaddr_in servaddr;                    // Socket address structure
    char buffer[BUFFER_SIZE];                       // Buffer to read from ) calls

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {                // Create a socket
        perror("Unable to create socket\n");                            // Error handling
        exit(1);    
    }
    memset(&servaddr, 0, sizeof(servaddr));                             // Clear the server socket address structure

    servaddr.sin_family = AF_INET;                                      // Set family to Internet
    servaddr.sin_port = htons(PORT_NO);                                 // Set port number
    inet_aton("127.0.0.1", &servaddr.sin_addr);                         // Set IP address to localhost

    printf("Enter DNS name: ");                                         // Prompt the user for a DNS name
    scanf("%s", buffer);                                                // Read the DNS name from the user
    int length = strlen(buffer);                                        // Get the length of the DNS name
    sendto(sockfd, buffer, length+1, 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));     // Send the DNS name to the server

    struct timeval tv;                              // Initialize a timeval structure
    tv.tv_sec = 2;                                  // Set the timeout to 2 seconds
    tv.tv_usec = 0;                                
    
    int flag = 0;
    while(1){   
        fd_set readfds;                                     // Initialize a file descriptor set
        FD_ZERO(&readfds);                                  // Clear the file descriptor set
        FD_SET(sockfd, &readfds);                           // Add the socket file descriptor to the set
        select(sockfd + 1, &readfds, NULL, NULL, &tv);      // Wait for the socket to be ready to read
        if (FD_ISSET(sockfd, &readfds)) {                   // If the socket is ready to read
            for(int i = 0; i < BUFFER_SIZE; i++){           // Clear the buffer
                buffer[i] = '\0';
            }
            int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&servaddr, &length);   // Read from the socket
            if (n < 0) {
                perror("Unable to read from socket\n");                                                // Error handling
                exit(1);
            }else{
                if(strcmp(buffer, "0.0.0.0") == 0){         // If DNS name is not found
                    break;
                }
                if(flag == 0){                              // If this is the first time the DNS name is found
                    printf("All the IP addresses: \n\n");
                    flag = 1;
                }
                printf("%s\n", buffer);                     // Print the IP address
            }
        }
        else {  
            if(flag == 0){                                  // If time out is reached before
                printf("Server busy: Could not connect\n");
                flag = 1;
            }
            break;
        }
    }
    if(flag == 0){
        printf("Invalid DNS name passed\nRecieved: %s\n", buffer);
    }
    printf("\n");
    close(sockfd);
    return 0;
}