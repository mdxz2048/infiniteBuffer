/*
 * @Author       : lvzhipeng
 * @Date         : 2023-08-25 16:08:02
 * @LastEditors  : lvzhipeng
 * @LastEditTime : 2023-09-05 10:06:02
 * @FilePath     : /lib_infiniteBuffer/lib_infinite_buffer_test_processRecv.c
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

#define NUM_THREADS 1

infiniteBuffer_t buffer;

void *consumer_thread(void *arg)
{
    int ret_len;
    testDataHeader_t header;
    char data[20480];
    u_int32_t remain_length = 0;
    int32_t bytesRead = 0;
    while (1)
    {

        bytesRead = lib_infinite_buffer_read_wait(&buffer, (char *)&header, sizeof(testDataHeader_t));
        printf("[RECV] bytesRead=[%d]\n", bytesRead);
        if (bytesRead <= 0)
        {
            printf("[RECV] buffer is empty\n");
        }
        else if (bytesRead == sizeof(testDataHeader_t))
        {
            if (header.sync == 0x55AA)
            {
                bytesRead = lib_infinite_buffer_read_wait(&buffer, data, header.len);
                printf("[RECV] msgCnt=[%ld], len=[%d]\n", header.msgCnt, header.len);
            }
            else
            {
                printf("[RECV] sync error\n");
            }
        }else 
        {
            printf("[RECV] header error\n");
        }
    }
    return NULL;
}

int main()
{
    int server_sock, client_sock;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_addr_len;

    // Initialize the buffer
    if (lib_infinite_buffer_create(&buffer, MAX_DATA_SIZE * 100) != SUCCESS)
    {
        perror("Failed to initialize buffer");
        exit(EXIT_FAILURE);
    }

    // Create the socket
    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, DOMAIN_SOCK_PATH, sizeof(server_addr.sun_path) - 1);

    unlink(DOMAIN_SOCK_PATH);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 5) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for connection...\n");
    client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_sock == -1)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // Start consumer threads
    pthread_t threads[NUM_THREADS];
    for (long i = 0; i < NUM_THREADS; i++)
    {
        if (pthread_create(&threads[i], NULL, consumer_thread, (void *)i) != 0)
        {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
    }

    // Main loop to receive data and write to buffer
    char recvBuffer[MAX_DATA_SIZE];
    uint64_t total_recv_length = 0;
    while (1)
    {
        ssize_t bytesReceived = recv(client_sock, recvBuffer, sizeof(recvBuffer), 0);
        if (bytesReceived <= 0)
        {
            if (bytesReceived == 0 || (bytesReceived == -1 && errno != EAGAIN && errno != EWOULDBLOCK))
            {
                perror("recv");
                break;
            }
        }
        else
        {
            total_recv_length += bytesReceived;
            if (lib_infinite_buffer_write_wait(&buffer, recvBuffer, bytesReceived) != SUCCESS)
            {

                fprintf(stderr, "Failed to write to buffer\n");
            }
            else
            {
                // printf("total_recv_length = %ld\n", total_recv_length);
                printf("[APP] write to buffer success.length=[%ld]\n", bytesReceived);
            }
        }
    }
    close(client_sock);
    close(server_sock);
    lib_infinite_buffer_destroy(&buffer);

    return 0;
}