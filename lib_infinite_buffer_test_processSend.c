/*
 * @Author       : lvzhipeng
 * @Date         : 2023-08-25 10:13:03
 * @LastEditors  : lvzhipeng
 * @LastEditTime : 2024-01-11 13:48:41
 * @FilePath     : /infiniteBuffer/lib_infinite_buffer_test_processSend.c
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

void sendData(int message_delay_us, int total_messages)
{
    struct sockaddr_un addr;
    char buf[MAX_DATA_SIZE];
    int sock_fd;
    struct timespec ts;

    ts.tv_sec = message_delay_us / 1000000;
    ts.tv_nsec = (message_delay_us % 1000000) * 1000;

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

    for (int i = 0; i < total_messages && isRunning; i++)
    {
        testDataHeader_t header;
        header.sync = SYNC_VALUE;
        header.msgCnt = i;                                                // 使用循环的迭代次数作为消息计数
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

        if (nanosleep(&ts, NULL) < 0)
        { // 在每个消息之间等待
            perror("nanosleep error");
            break;
        }
    }

    sleep(5);
    close(sock_fd);
}
void handle_sigint(int sig)
{
    printf("\nCaught signal %d! Stopping...\n", sig);
    isRunning = false;
}

int main(int argc, char *argv[])
{
    int message_delay_us = 0;
    int total_messages = 0;
    // 解析命令行参数
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--message_delay_us") == 0 && i + 1 < argc)
        {
            message_delay_us = atoi(argv[i + 1]);
            i++; // 跳过下一个参数（已读取的值）
        }
        else if (strcmp(argv[i], "--total_messages") == 0 && i + 1 < argc)
        {
            total_messages = atoi(argv[i + 1]);
            i++; // 跳过下一个参数（已读取的值）
        }
        else
        {
            fprintf(stderr, "unknown : %s\n", argv[i]);
            return 1;
        }
    }
    printf("Process send start\n");
    // 设置SIGINT信号处理器
    signal(SIGINT, handle_sigint);
    sendData(message_delay_us, total_messages);
    printf("Process send exit\n");
    fprintf(stderr, "[PYTHON_RESULT_FLAG]  TotalMessagesSent=%d\n", total_messages);

    return 0;
}