#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "list.h"

#define MAX_LENGTH 100

static char *port_number;
static char *remote_port;
static char *remote_machine;

static int port_number_int;
static int remote_port_int;

static List *in_list;
static List *out_list;

static pthread_mutex_t output_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t output_cond_var = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t input_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t input_cond_var = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t main_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t main_cond_var = PTHREAD_COND_INITIALIZER;

static int in_list_has_data = 0;
static int out_list_has_data = 0;

// Function for Free_list() to call
void freeMessages(void* message) {
	return;
}

// Read input from keyboard
void *input_from_keyboard(void *in_list)
{
    char buffer[MAX_LENGTH]; // Array to store the input string

    while(1) {
        fgets(buffer, MAX_LENGTH, stdin); // Read string from input

		// Main program exits when user input "!"
        if (*(buffer) == '!' && *(buffer + 2) == '\0'){
            pthread_mutex_lock(&main_lock);
            pthread_cond_signal(&main_cond_var);
            pthread_mutex_unlock(&main_lock);
        }

        // Lock thread
        pthread_mutex_lock(&input_lock);

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
        pthread_cond_signal(&input_cond_var);

        // Unlock thread
        pthread_mutex_unlock(&input_lock);
    }

	return NULL;
}

// Write output to the screen
void *output_to_screen(void *out_list) {
    while (1) {

        pthread_mutex_lock(&output_lock);

		// Prevent from proceeding until out_list has data
        while(!out_list_has_data)
        {
            pthread_cond_wait(&output_cond_var, &output_lock);
        }

        // Take message out of list
        char outputBuffer[MAX_LENGTH];
        strcpy(outputBuffer, (char *)List_trim(out_list));

        // Non Critical section
        // Process received data
        printf("[%d]: %s\n", remote_port_int, outputBuffer);

        out_list_has_data = 0; // out_list is now empty
        pthread_mutex_unlock(&output_lock);

    }

    return NULL;
}

// Receive message from remote
void *receive_udp_in(void *out_list) {
    struct addrinfo hints, *res, *p;
    int sockfd;
    char receivedBuffer[MAX_LENGTH];

    // Set up hints structure
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // Use IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket
    hints.ai_flags = AI_PASSIVE; // For wildcard IP address

    // Get address information
    if (getaddrinfo(NULL, &port_number[0], &hints, &res) != 0) {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    // Loop through all the results and bind to the first one we can
    for (p = res; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "Failed to bind socket\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res); // Free the linked list

    // Receive and respond to client messages
    while (1) {
        // Receive data from remote
        ssize_t recvBytes = recvfrom(sockfd, receivedBuffer, MAX_LENGTH, 0, p->ai_addr, &p->ai_addrlen);

		// Calculate terminated character index
		int terminatedChar = (recvBytes < MAX_LENGTH) ? recvBytes : (MAX_LENGTH - 1);
        receivedBuffer[terminatedChar] = '\0';

		// Terminate when receive "!" signal
        if (*(receivedBuffer) == '!' && *(receivedBuffer + 2) == '\0'){
            pthread_mutex_lock(&main_lock);
            pthread_cond_signal(&main_cond_var);
            pthread_mutex_unlock(&main_lock);
        }
        else{
            pthread_mutex_lock(&output_lock);

            // Start Critical Section
            // Put message into output list
            List_prepend(out_list, &receivedBuffer);

            out_list_has_data = 1;
            pthread_cond_signal(&output_cond_var);

			// End Critical Section
            pthread_mutex_unlock(&output_lock);
        }
    }
    // Close the socket
    close(sockfd);
}

// Send message to remote
void *send_udp_out(void *in_list) {
    struct addrinfo hints, *res, *p;
    int sockfd;
    char buffer[MAX_LENGTH];

    // Set up hints structure
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // Use IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM; // UDP socket

    // Get address information
    if (getaddrinfo(remote_machine, &remote_port[0], &hints, &res) != 0) {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    // Loop through all the results and connect to the first one we can
    for (p = res; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        break;
    }

    if (res == NULL) {
        fprintf(stderr, "Failed to create socket\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res); // Free the linked list

    // User input
    while(1){
        // Lock thread
        pthread_mutex_lock(&input_lock);

        // Prevent from proceeding until there is data available
        while(!in_list_has_data) {
            pthread_cond_wait(&input_cond_var, &input_lock);
        }

        // Read message from the list
        char *message = List_first(in_list);

        in_list_has_data = 0; // in_list is now empty

        // Unlock thread
        pthread_mutex_unlock(&input_lock);

        // Send data to remote
        sendto(sockfd, message, strlen(message), 0, p->ai_addr, p->ai_addrlen);
    }

    // Close the socket
    close(sockfd);
}

int main(int argc, char* argv[]) {
    // Get input arguments from user
    port_number = argv[1];
    remote_machine = argv[2];
    remote_port = argv[3];

    port_number_int = atoi(port_number);
    remote_port_int = atoi(remote_port);

    // Create input and output list
    in_list = List_create();
    out_list = List_create();

    // Initilize threads
    pthread_t keyboard_thread;
    pthread_t screen_thread;
    pthread_t udp_out;
    pthread_t udp_in;

    // Initialize mutex
    pthread_mutex_init(&input_lock, NULL);
    pthread_mutex_init(&output_lock, NULL);
    pthread_mutex_init(&main_lock, NULL);

    // Create threads
	pthread_create(&keyboard_thread, NULL, input_from_keyboard, (void *)in_list);
	pthread_create(&screen_thread, NULL, output_to_screen,(void *)out_list);
    pthread_create(&udp_in, NULL, receive_udp_in,(void *)out_list);
    pthread_create(&udp_out, NULL, send_udp_out, (void *)in_list);

    pthread_mutex_lock(&main_lock);
    pthread_cond_wait(&main_cond_var, &main_lock);
    pthread_mutex_unlock(&main_lock);

    printf("Program exiting\n");

    // Pthread cancel
    pthread_cancel(keyboard_thread);
    pthread_cancel(screen_thread);
    pthread_cancel(udp_in);
    pthread_cancel(udp_out);

    // Join threads
    pthread_join(keyboard_thread, NULL);
    pthread_join(screen_thread, NULL);
    pthread_join(udp_in, NULL);
    pthread_join(udp_out, NULL);

    // Mutex destroy
    pthread_mutex_destroy(&input_lock);
    pthread_mutex_destroy(&output_lock);
    pthread_mutex_destroy(&main_lock);

    // Cond var destroy
    pthread_cond_destroy(&input_cond_var);
    pthread_cond_destroy(&output_cond_var);
    pthread_cond_destroy(&main_cond_var);

	// List free
	List_free(in_list, &freeMessages);
	List_free(out_list, &freeMessages);

    exit(0);
}
