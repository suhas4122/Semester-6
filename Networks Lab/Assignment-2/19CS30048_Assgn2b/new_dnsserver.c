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

int max(int num1, int num2)
{
    return (num1 > num2 ) ? num1 : num2;
}

int main(){
    int udp_sockfd;                                         // UDP socket file descriptor
    struct sockaddr_in servaddr, cliaddr;                   // Socket address structures
    char buffer[BUFFER_SIZE];                               // Buffer to read from recvfrom() calls

    udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);            // Create a UDP socket
    if (udp_sockfd < 0) {                                                                       
        perror("Unable to create socket");                  // Error handling
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));                 // Clear the server socket address structure
    memset(&cliaddr, 0, sizeof(cliaddr));                   // Clear the client socket address structure

    servaddr.sin_family = AF_INET;                          // Set family to Internet
    servaddr.sin_port = htons(PORT_NO);                     // Set port number
    servaddr.sin_addr.s_addr = INADDR_ANY;                  // Set IP address to localhost

    if(bind(udp_sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {        // Bind the socket to the address
        perror("Unable to bind");                                                           // Error handling
        exit(1);
    }

	int tcp_sockfd, tcp_newsockfd ;                         // TCP socket file descriptors 
	int clilen;                                             // Length of the client address structure

	if ((tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {                               // Create a socket
		printf("Cannot create socket\n");                                                   // Error handling
		exit(0);    
	}

	if (bind(tcp_sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {            // Bind the socket to the address
		printf("Unable to bind\n");                                                         // Error handling
		exit(0);    
	}

    clilen = sizeof(cliaddr);                                 // Get the length of the client address structure
	listen(tcp_sockfd, 5);                                    // Listen for atmost 5 TCP connections in one socket
    printf("Server Running....\n\n");

	while(1){
        fd_set readfds;                                                         // Set of file descriptors to be checked for readability
        FD_ZERO(&readfds);                                                      // Clear the set
        FD_SET(tcp_sockfd, &readfds);                                           // Add the socket to the set
        FD_SET(udp_sockfd, &readfds);                                           // Add the socket to the set
        select(max(udp_sockfd, tcp_sockfd) + 1, &readfds, NULL, NULL, NULL);    // Wait for readability on the set

        if (FD_ISSET(tcp_sockfd, &readfds)) {                                   // If TCP socket is readable

            tcp_newsockfd = accept(tcp_sockfd, (struct sockaddr *) &cliaddr, &clilen);      // Accept a connection
            if (tcp_newsockfd < 0) {                                                        // Error handling
                printf("Unable to accept\n");
                exit(0);
            }
            printf("Connected to TCP client\n\n");                                        

            if(fork() == 0){                                 // Divide into parent and child processes                      
                close(tcp_sockfd);                           // Close the parent TCP socket
                close(udp_sockfd);	                         // Close the UDP socket

                for(int i = 0; i < BUFFER_SIZE; i++){
                    buffer[i] = '\0';
                }

                int n = 0;                                   // Number of bytes read from recvfrom()
                while(1){
                    n += recv(tcp_newsockfd, buffer + n, BUFFER_SIZE-n, 0);         // Read from the socket
                    if(buffer[n-1] == '\0')                                         // If the last character is a null character
                        break;
                }
                printf("Data recieved from TCP client\n\n");
                printf("DNS name: %s\n\n", buffer);

                struct hostent *he;                           // Structure to store host information
                struct in_addr **addr_list;                   // List of IP addresses                 
                struct in_addr addr;                          // IP address structure
                he = gethostbyname(buffer);                   // Get the host information

                if(he == NULL){                               // If the host information is not found
                    char *error = "0.0.0.0\nInvalid DNS name passed\0\0";
                    send(tcp_newsockfd, error, strlen(error)+2, 0);
                    herror("gethostbyname");
                    printf("\n");
                    exit(1);
                }

                addr_list = (struct in_addr **)he->h_addr_list;     // Get the list of IP addresses
                for(int i = 0; addr_list[i] != NULL; i++) {         // Loop through the list of IP addresses
                    char* ip = inet_ntoa(*addr_list[i]);
                    send(tcp_newsockfd, ip, strlen(ip)+1, 0);       // Send the IP address to the client
                }
                send(tcp_newsockfd, "\0", 1, 0);                    // Send a null character to the client to denote end of message
                printf("DNS addresses sent to TCP client\n\n");
                close(tcp_newsockfd);                               // Close the socket
                exit(0);
            }

            close(tcp_newsockfd);                                   // Close the child TCP socket
        }
        if(FD_ISSET(udp_sockfd, &readfds)){
            for(int i = 0; i < BUFFER_SIZE; i++){
                buffer[i] = '\0';
            }

            int n = recvfrom(udp_sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&cliaddr, &clilen);     // Receive from the client
            if(n < 0) {
                perror("Unable to read from socket\n");                                                     // Error handling
            }
            printf("Data recieved from UDP client\n\n");
            printf("DNS name: %s\n\n", buffer);

            struct hostent *he;                           // Structure to store host information
            struct in_addr **addr_list;                   // List of IP addresses                 
            struct in_addr addr;                          // IP address structure
            he = gethostbyname(buffer);                   // Get the host information

            if(he == NULL){                               // If the host information is not found  
                sendto(udp_sockfd, "0.0.0.0", strlen("0.0.0.0"), 0, (const struct sockaddr *) &cliaddr, sizeof(cliaddr));
                herror("gethostbyname"); 
                printf("\n");
            }else{
                addr_list = (struct in_addr **)he->h_addr_list;         // Get the list of IP addresses
                for(int i = 0; addr_list[i] != NULL; i++) {             // Loop through the list of IP addresses
                    char* ip = inet_ntoa(*addr_list[i]);
                    sendto(udp_sockfd, ip, strlen(ip), 0, (const struct sockaddr *) &cliaddr, sizeof(cliaddr));    // Send the IP address to the client
                }
                printf("DNS addresses sent to UDP client\n\n");
            }
        }		
	}
}
