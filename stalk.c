#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "list.h"

#define MAX_LENGTH 100
#define SERVER_IP "127.0.0.1"

static char port_number[MAX_LENGTH];
static char remote_port[MAX_LENGTH];
static int port_number_int;
static int remote_port_int;

static List *in_list;
static List *out_list;

// Read input from keyboard
void *input_from_keyboard(void *in_list)
{
    char buffer[MAX_LENGTH]; // Array to store the input string
    printf("Please type message to send: ");
    fgets(buffer, MAX_LENGTH, stdin); // Read string from keyboard

    // Clear in_list
    int list_size = List_count(in_list);
    List_first(in_list); // Go to first item in the in_list
    for (int i = 0; i < list_size; i++) {
        void* result = List_remove(in_list);
    }

    // Save input from buffer to in_list
    List_append(in_list, &buffer);

    // Test: print out in_list
    char *message = List_first(in_list); // Get message from in_list
    fputs((char*)message, stdout);

	return NULL;
}

// Write output to the screen
void *output_to_screen(void *out_list) {
    char *message = List_first(out_list); // Get message from buffer
    fputs((char*)message, stdout); // Print message to screen
}

// Receive message from remote
void *receive_udp_in(void *in_list) {
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_size;
    char buffer[MAX_LENGTH];

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address structure
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_number_int);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to address and port
    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    while(1){
        // Receive data from remote
        addr_size = sizeof(clientAddr);
        ssize_t recvBytes = recvfrom(sockfd, buffer, MAX_LENGTH, 0, (struct sockaddr*)&clientAddr, &addr_size);
        buffer[recvBytes] = '\0';

        // Process received data
        printf("[%d]: %s\n", remote_port_int, buffer);

    }
    // Close the socket
    close(sockfd);
}

//send message to remote
void *send_udp_out(void *out_list) {
    int sockfd;
    struct sockaddr_in serverAddr;
    char buffer[MAX_LENGTH];

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Configure udp out address structure
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(remote_port_int);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // User input
    while(1){
        printf("[%d]: ", port_number_int);
        // TO-DO: Read from the list - code down here
        fgets(buffer, MAX_LENGTH, stdin);

        // Send data to remote
        sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    }

    // Close the socket
    close(sockfd);
}

int main() {
    // Get port numbers from keyboard
    printf("Port number: ");
    fgets(port_number, MAX_LENGTH, stdin); // Read string from keyboard
    printf("Remote port number: ");
    fgets(remote_port, MAX_LENGTH, stdin); // Read string from keyboard

    remote_port_int = atoi(remote_port);
    port_number_int = atoi(port_number);

    // Create input and output list
    in_list = List_create();
    out_list = List_create();

    // Initilize threads
    pthread_t keyboard_thread;
    pthread_t screen_thread;
    pthread_t udp_out;
    pthread_t udp_in;

    // Create threads
	//pthread_create(&keyboard_thread, NULL, input_from_keyboard, (void *)in_list);
	//pthread_create(&screen_thread, NULL, output_to_screen,(void *)buffer);
    pthread_create(&udp_in, NULL, receive_udp_in,(void *)in_list);
    pthread_create(&udp_out, NULL, send_udp_out,NULL);

    // Join threads
    //pthread_join(keyboard_thread, NULL);
    //pthread_join(screen_thread, NULL);
    pthread_join(udp_in, NULL);
    pthread_join(udp_out, NULL);

	exit(0);
}
