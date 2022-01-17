#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 100                                             // Size of the buffer to read from text file
#define PORT_NO 20200                                               // Port number to connect to

struct output {                                                     // Struct to store the output
    int sentence, word, character;
};

int main(int argc, char* argv[]) {                                  
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

    printf("Connected to server...\n\n");

    int open_file = open(argv[1], O_RDONLY);                        // Open the file from the command line argument
    if (open_file < 0) {
        perror("Unable to open file\n");                            // Error handling
        exit(1);
    }

    int flag = 1;
    while(flag){                                                    // Loop until the end of the file
        int length = read(open_file, buffer, BUFFER_SIZE);          // Read from the file
        if(length < BUFFER_SIZE){                                   
            buffer[length] = '.';                                   // Add a period to the end of the file
            flag = 0;
            send(sockfd, buffer, length + 1, 0);                    // Send to the server
        }else{
            send(sockfd, buffer, BUFFER_SIZE, 0);                   // Send to the server
        }        
    }

    struct output* counter = (struct output*)malloc(sizeof(struct output));     // Allocate memory for the counter struct
    counter->sentence = counter->word = counter->character = 0;                 // Initialize the counter

    int n = 0;
    while(n < sizeof(struct output)){                                           // Receive from the server
        n += recv(sockfd, counter+n, sizeof(struct output), 0);                 // Receive from the server
    }

    printf("Number of characters in file: %d\n", counter->character);           // Print the number of characters
    printf("Number of words in file: %d\n", counter->word);                     // Print the number of words
    printf("Number of sentences in file: %d\n\n", counter->sentence);           // Print the number of sentences   

    close(sockfd);
}