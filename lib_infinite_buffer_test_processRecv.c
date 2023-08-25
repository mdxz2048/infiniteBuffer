/*
 * @Author       : lvzhipeng
 * @Date         : 2023-08-25 16:08:02
 * @LastEditors  : lvzhipeng
 * @LastEditTime : 2023-08-25 16:38:52
 * @FilePath     : /lib_caeri_infiniteBuffer/lib_infinite_buffer_test_processRecv.c
 * @Description  : 
 * 
 */

#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/timerfd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <errno.h>
#include "lib_infinite_buffer.h"
#include "lib_infinite_buffer_test.h"


#define NUM_THREADS 4

infiniteBuffer_t buffer;

void *consumer_thread(void *arg) {
    char data[MAX_DATA_SIZE];
    while (1) {
        int32_t bytesRead = lib_infinite_buffer_read(&buffer, data, MAX_DATA_SIZE);
        if (bytesRead > 0) {
            // Process the data as needed
            // For now, just print it
            printf("Thread %ld: Received %d bytes, msgCnt =[%ld]\n", (long)arg, bytesRead, ((testDataHeader_t *)data)->msgCnt);
        } else {
            // Handle error or wait for data
            usleep(1000);  // Sleep for 1ms
        }
    }
    return NULL;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_addr_len;

    // Initialize the buffer
    if (lib_infinite_buffer_create(&buffer, MAX_DATA_SIZE * 100) != SUCCESS) {
        perror("Failed to initialize buffer");
        exit(EXIT_FAILURE);
    }

    // Create the socket
    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, DOMAIN_SOCK_PATH, sizeof(server_addr.sun_path) - 1);

    unlink(DOMAIN_SOCK_PATH);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 5) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for connection...\n");
    client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_sock == -1) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // Start consumer threads
    pthread_t threads[NUM_THREADS];
    for (long i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, consumer_thread, (void *)i) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
    }

    // Main loop to receive data and write to buffer
    char recvBuffer[MAX_DATA_SIZE];
    while (1) {
        ssize_t bytesReceived = recv(client_sock, recvBuffer, sizeof(recvBuffer), 0);
        if (bytesReceived <= 0) {
            if (bytesReceived == 0 || (bytesReceived == -1 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                perror("recv");
                break;
            }
        } else {
            if (lib_infinite_buffer_write(&buffer, recvBuffer, bytesReceived) != SUCCESS) {
                fprintf(stderr, "Failed to write to buffer\n");
            }
        }
    }

    close(client_sock);
    close(server_sock);
    lib_infinite_buffer_destroy(&buffer);

    return 0;
}