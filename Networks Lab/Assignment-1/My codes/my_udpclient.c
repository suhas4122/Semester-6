#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 100                             // Size of the buffer to read from the file
#define PORT_NO 20200                               // Port number to connect to

struct output {                                     // Struct to store the output
    int sentence, word, character;                  // Number of sentences, words and characters
};

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

    int open_file = open(argv[1], O_RDONLY);                            // Open the file from the command line argument
    if (open_file < 0) {
        perror("Unable to open file\n");                                // Error handling
        exit(1);
    }

    printf("Client Running....\n\n");

    int flag = 1;                                                       // Flag to stop while loop
    while(flag){
        int length = read(open_file, buffer, BUFFER_SIZE);              // Read from the file
        if(length < BUFFER_SIZE){                                       
            buffer[length] = '.';                                       // Add a period to the end of the file
            flag = 0;   
            sendto(sockfd, buffer, length+1, 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));     // Send to the server
        }else{
            sendto(sockfd, buffer, BUFFER_SIZE, 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));   // Send to the server
        }        
    }

    struct output* counter = (struct output*)malloc(sizeof(struct output));  // Allocate memory for the counter 
    counter->sentence = counter->word = counter->character = 0;              // Initialize the counter

    socklen_t len = sizeof(servaddr);                                                           // Length of the client address structure
    recvfrom(sockfd, counter, sizeof(struct output), 0, (struct sockaddr *)&servaddr, &len);    // Receive the counter from the server

    printf("Number of characters in file: %d\n", counter->character);                           // Print the number of characters
    printf("Number of words in file: %d\n", counter->word);                                     // Print the number of words
    printf("Number of sentences in file: %d\n\n", counter->sentence);                           // Print the number of sentences

    close(sockfd);
    return 0;
}