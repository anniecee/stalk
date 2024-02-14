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
static char remote_machine[MAX_LENGTH];
static int port_number_int;
static int remote_port_int;

static List *in_list;
static List *out_list;

static pthread_mutex_t output_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t output_cond_var = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;

static int in_list_has_data = 0;

// Read input from keyboard
void *input_from_keyboard(void *in_list)
{
    char buffer[MAX_LENGTH]; // Array to store the input string

    while (1)
    {
        printf("[%d]: ", port_number_int);

        fgets(buffer, MAX_LENGTH, stdin); // Read string from keyboard

        // Lock thread
        pthread_mutex_lock(&lock);

        // Clear in_list
        int list_size = List_count(in_list);
        List_first(in_list); // Go to first item in the in_list
      
        for (int i = 0; i < list_size; i++) {
            void* result = List_remove(in_list);
        }

        // Save input from buffer to in_list
        List_append(in_list, &buffer);

        // Signal to wake up output_to_screen
        in_list_has_data = 1;
        pthread_cond_signal(&cond_var);

        // Unlock thread
        pthread_mutex_unlock(&lock);
    }

    return NULL;
}

// Write output to the screen
void *output_to_screen(void *out_list) {
    while(1){
        //signalOutputMsg()

        pthread_mutex_lock(&output_lock);
        // Critical section

        if(List_count(out_list) == 0)
        {
            pthread_cond_wait(&output_cond_var, &output_lock);
        }

        // Take message out of list
        char outputBuffer[MAX_LENGTH];
        strcpy(outputBuffer, (char *)List_trim(out_list));

        // Non Critical section
        // Process received data 
        printf("[%d]: %s\n", remote_port_int, outputBuffer);

        if (*(outputBuffer) == '!'){
            // Shut down thread
        }
        pthread_mutex_unlock(&output_lock);

        // Signal a node is available
    }
}

// Receive message from remote
void *receive_udp_in(void *out_list)
{
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_size;
    char receivedBuffer[MAX_LENGTH];

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
        ssize_t recvBytes = recvfrom(sockfd, receivedBuffer, MAX_LENGTH, 0, (struct sockaddr *)&clientAddr, &addr_size);
        int terminatedChar = (recvBytes < MAX_LENGTH) ? recvBytes : (MAX_LENGTH - 1);
        receivedBuffer[terminatedChar] = '\0';

        pthread_mutex_lock(&output_lock);

        // Start Critical Section
        // Put message into output list
        List_prepend(out_list, &receivedBuffer);
        pthread_cond_signal(&output_cond_var);

        pthread_mutex_unlock(&output_lock);
    }
    // Close the socket
    close(sockfd);
}

// send message to remote
void *send_udp_out(void *in_list)
{
    int sockfd;
    struct sockaddr_in serverAddr;
    char buffer[MAX_LENGTH];

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Configure udp out address structure
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(remote_port_int);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // User input
    while (1)
    {
        // Lock thread
        pthread_mutex_lock(&lock);

        // Prevent from proceeding until there is data available

        while(!in_list_has_data) {
            pthread_cond_wait(&cond_var, &lock);
        }

        // Read message from the list
        char *message = List_first(in_list);

        in_list_has_data = 0; // in_list is now empty

        // Unlock thread
        pthread_mutex_unlock(&lock);

        // Send data to remote

        sendto(sockfd, message, strlen(message), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
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
    printf("Remote machine name: ");
    fgets(remote_machine, MAX_LENGTH, stdin); // Read string from keyboard

    remote_port_int = atoi(remote_port);
    port_number_int = atoi(port_number);
    remote_machine[strcspn(remote_machine, "\n")] = 0; // Remove newline

    // Create input and output list
    in_list = List_create();
    out_list = List_create();

    // Initilize threads
    pthread_t keyboard_thread;
    pthread_t screen_thread;
    pthread_t udp_out;
    pthread_t udp_in;

    // Initialize mutex
    pthread_mutex_init(&lock, NULL);

    // Create threads
	  pthread_create(&keyboard_thread, NULL, input_from_keyboard, (void *)in_list);
	  pthread_create(&screen_thread, NULL, output_to_screen,(void *)out_list);
    pthread_create(&udp_in, NULL, receive_udp_in, (void *)out_list);
    pthread_create(&udp_out, NULL, send_udp_out, (void *)in_list);

    // Join threads
    pthread_join(keyboard_thread, NULL);
    pthread_join(screen_thread, NULL);
    pthread_join(udp_in, NULL);
    pthread_join(udp_out, NULL);

    pthread_mutex_destroy(&lock);

    exit(0);
}
