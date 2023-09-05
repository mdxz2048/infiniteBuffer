/*
 * @Author       : lvzhipeng
 * @Date         : 2023-08-25 10:13:03
 * @LastEditors  : lvzhipeng
 * @LastEditTime : 2023-09-05 14:40:06
 * @FilePath     : /lib_infiniteBuffer/lib_infinite_buffer_test_processSend.c
 * @Description  :
 *
 */
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/timerfd.h>
#include <time.h>
#include "lib_infinite_buffer.h"
#include "lib_infinite_buffer_test.h"

#define TIMER_INTERVAL_SEC 1  // 定时器间隔，秒
#define TIMER_INTERVAL_NSEC 0 // 定时器间隔，纳秒

bool isRunning = true;

void sendData()
{
    struct sockaddr_un addr;
    char buf[MAX_DATA_SIZE];
    int sock_fd, timer_fd;
    u_int64_t messageCount = 0; // Message counter

    if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("socket error");
        exit(-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, DOMAIN_SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("connect error");
        exit(-1);
    }

    // 创建timerfd
    timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timer_fd == -1)
    {
        perror("timerfd_create error");
        exit(-1);
    }

    // 设置定时器间隔为1秒
    struct itimerspec timer_value;
    timer_value.it_value.tv_sec = 2;
    timer_value.it_value.tv_nsec = 0;
    timer_value.it_interval.tv_sec = 0;
    timer_value.it_interval.tv_nsec = 1 * 100000;

    // 启动定时器
    if (timerfd_settime(timer_fd, 0, &timer_value, NULL) == -1)
    {
        perror("timerfd_settime error");
        exit(-1);
    }

    uint64_t expirations;
    uint64_t total_send_length = 0;
    while (isRunning)
    {
        // 等待定时器事件
        if (read(timer_fd, &expirations, sizeof(expirations)) != sizeof(expirations))
        {
            perror("read timer_fd error");
            exit(-1);
        }

        testDataHeader_t header;
        header.sync = SYNC_VALUE;
        header.msgCnt = messageCount++;                                   // Increment the message counter
        header.len = rand() % (MAX_DATA_SIZE - sizeof(testDataHeader_t)); // Generate random length
        printf("[SEND] msgCnt = %ld, len = %d\n", header.msgCnt, header.len);
        memcpy(buf, &header, sizeof(header));
        // Fill the buffer with some data after the header
        for (int i = sizeof(header); i < header.len + sizeof(header); i++)
        {
            buf[i] = rand() % 256; // Random byte
        }

        if (write(sock_fd, buf, header.len + sizeof(header)) == -1)
        {
            perror("write error");
            exit(-1);
        }
        total_send_length += header.len + sizeof(header);
    }
    printf("total_send_length = %ld\n", total_send_length);
    sleep(10);
    close(sock_fd);
    close(timer_fd);
}
void handle_sigint(int sig)
{
    printf("\nCaught signal %d! Stopping...\n", sig);
    isRunning = false;
}

int main()
{
    printf("Process A\n");
    // 设置SIGINT信号处理器
    signal(SIGINT, handle_sigint);
    sendData();
    printf("Process A exit\n");
    return 0;
}