#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 100                     // Size of the buffer to read from recvfrom() calls
#define PORT_NO 20200                       // Port number to connect to

struct output {                             // Struct to store the output
    int sentence, word, character;          // Number of sentences, words and characters
};

int alphanum(char c){                       // Function to check if a character is alphanumeric
    if(c >= 'a' && c <= 'z')
        return 1;
    if(c >= 'A' && c <= 'Z')
        return 1;
    if(c >= '0' && c <= '9')
        return 1;
    return 0;
}

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

    socklen_t len = sizeof(cliaddr);                                                  // Length of the client address structure
    printf("Server Running....\n");
    
    struct output* counter = (struct output*)malloc(sizeof(struct output));           // Allocate memory for the counter
    counter->sentence = counter->word = counter->character = 0;                       // Initialize the counter

    char prev = ' ';                                                                  // Previous character
    int flag = 1;                                                                     // Flag to stop while loop
    while(flag) {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&cliaddr, &len);       // Receive from the client
        if(n < 0) {
            perror("Unable to read from socket\n");                                                // Error handling
            exit(1);
        }
        for(int i = 0; i < n; i++){                                 // Loop through the buffer
            counter->character++;                                   // Increment the character counter
            if(prev == '.' && buffer[i] == '.') {                   // Detection of end of file using two periods 
                counter->character--;                               // To not count the last period
                flag = 0;                                           // Set flag to stop while loop
                break;
            }
            if(buffer[i] == '.') {                                  // Detection of end of sentence
                counter->sentence++;
            }
            if(!alphanum(prev) && alphanum(buffer[i])) {            // Detection of start of word
                counter->word++;
            }
            prev = buffer[i];                                       // Update the previous character
        }
    }

    sendto(sockfd, counter, sizeof(struct output), 0, (const struct sockaddr *)&cliaddr, len);       // Send the counter to the client
    printf("Output sent to client\n\n");

    close(sockfd);                                                  // Close the socket
    return 0;    
}