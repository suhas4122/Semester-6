#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>

#define BUFFER_SIZE 100                                             // Size of the buffer to read from text file
#define PORT_NO 20200                                               // Port number to connect to

int main() {                                  
    int sockfd;                                                     // Socket file descriptor
    struct sockaddr_in servaddr;                                    // Socket address structure
    char buffer[BUFFER_SIZE];                                       // Buffer to read from text file

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {           // Create a socket
        perror("Unable to create socket\n");                        // Error handling
        exit(1);
    }   
    memset(&servaddr, 0, sizeof(servaddr));                         // Clear the server socket address structure

    servaddr.sin_family = AF_INET;                                  // Set family to Internet
    servaddr.sin_port = htons(PORT_NO);                             // Set port number
    inet_aton("127.0.0.1", &servaddr.sin_addr);                     // Set IP address to localhost
    
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {         // Connect to the server
        perror("Unable to connect to server\n");                                        // Error handling
        exit(1);
    }

    printf("Connected to server...\n\n");                           // Print a message to the user

    printf("Enter DNS name: ");                                     // Prompt the user for a DNS name
    scanf("%s", buffer);                                            // Read the DNS name from the user
    int length = strlen(buffer);                                    // Get the length of the DNS name
    send(sockfd, buffer, length+1, 0);                              // Send the DNS name to the server

    char prev = ' ';                                                // Initialize few variables
    int flag = 1;                                                  
    int first = 0;
    while(flag){
        for(int i = 0; i < BUFFER_SIZE; i++){                       // Clear the buffer
            buffer[i] = '\0';
        }
        int n = recv(sockfd, buffer, BUFFER_SIZE, 0);               // Read from the socket
        if(first == 0 && n > 0){                                    // If the first time reading from the socket
            printf("All the IP addresses: \n\n");
            first = 1;
        }
        if (n < 0) {                                                // Error handling
            perror("Unable to read from socket\n");                                              
            exit(1);
        }else{
            for(int i = 0; i < n; i++){                             // Iterate through the buffer
                if(buffer[i] == '\0' && prev == '\0'){              // If end of message is detected
                    printf("\n");
                    flag = 0;
                    break;
                }
                else if(buffer[i] == '\0'){                         // If end of line is detected
                    printf("\n");
                }else{                                              // If the character is not a null character
                    printf("%c", buffer[i]);
                }
                prev = buffer[i];                                   // Update the previous character
            }
        }
    }
    
    close(sockfd);
}