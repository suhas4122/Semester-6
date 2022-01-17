#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 100                             // Size of the buffer to read from read() calls
#define PORT_NO 20200                               // Port number to connect to

struct output {                                     // Struct to store the output
    int sentence, word, character;          
};

int alphanum(char c){                               // Function to check if a character is alphanumeric
    if(c >= 'a' && c <= 'z')
        return 1;
    if(c >= 'A' && c <= 'Z')
        return 1;
    if(c >= '0' && c <= '9')
        return 1;
    return 0;
}

int main() {
    int sockfd, newsockfd;                                      // Socket file descriptor
    int clilen;                                                 // Length of the client address structure
    struct sockaddr_in cliaddr, servaddr;                       // Socket address structures
    char buffer[BUFFER_SIZE];                                   // Buffer to read from read() calls 

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {       // Create a socket
		perror("Unable to create socket\n");                    // Error handling
		exit(1);
	}

    memset(&servaddr, 0, sizeof(servaddr));                     // Clear the server socket address structure
    memset(&cliaddr, 0, sizeof(cliaddr));                       // Clear the client socket address structure

    servaddr.sin_family = AF_INET;                              // Set family to Internet
	servaddr.sin_addr.s_addr = INADDR_ANY;                      // Set IP address to localhost
	servaddr.sin_port = htons(PORT_NO);                         // Set port number

    if (bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {     // Bind the socket to the address
		perror("Unable to bind local address\n");                                // Error handling
		exit(1);
	}

    listen(sockfd, 5);                                                          // Maximum number of requests to queue

    while(1) {
        printf("Waiting for connection...\n\n");
        clilen = sizeof(cliaddr);                                               // Length of the client address structure
        newsockfd = accept(sockfd, (struct sockaddr *) &cliaddr, &clilen);      // Accept a connection
        if(newsockfd < 0) {                                                     // Error handling
            perror("Unable to accept connection\n");                            // Error handling
            exit(1);
        }

        struct output* counter = (struct output*)malloc(sizeof(struct output));       // Allocate memory for the counter
        counter->sentence = counter->word = counter->character = 0;                   // Initialize the counter

        char prev = ' ';                                            // Initialize the previous character
        int flag = 1;                                               // Flag to stop the loop
        while(flag) {                                               
            int n = read(newsockfd, buffer, BUFFER_SIZE);           // Read from the socket
            if(n < 0) {
                perror("Unable to read from socket\n");             // Error handling
                exit(1);
            }
            for(int i = 0; i < n; i++){                             // Loop over all characters in the buffer
                counter->character++;                   
                if(prev == '.' && buffer[i] == '.') {               // Detect end of file
                    counter->character--;                          
                    flag = 0;
                    break;
                }
                if(buffer[i] == '.') {                              // Detect end of sentence
                    counter->sentence++;
                }
                if(!alphanum(prev) && alphanum(buffer[i])) {        // Detect start of word
                    counter->word++;
                }
                prev = buffer[i];                                   // Update the previous character
            }
        }

        send(newsockfd, counter, sizeof(struct output), 0);         // Send the counter to the client
        close(newsockfd);                                           // Close the socket
    }
}