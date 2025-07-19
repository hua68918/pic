#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>  // 添加时间相关头文件
#include <unistd.h> // 保留原有头文件

#define SOCKET_PATH "/tmp/zoom_sdk_trans_video_receive_82798372765.sock"

void print_time_ms() {
    struct timeval tv;
    struct tm *local_time;
    char time_str;

    // 获取当前时间（微秒级精度）
    gettimeofday(&tv, NULL);  // ‌:ml-citation{ref="3,7" data="citationList"}

    // 转换为本地时间结构体
    local_time = localtime(&tv.tv_sec);  // ‌:ml-citation{ref="3" data="citationList"}

    // 格式化时间字符串（含毫秒）
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);
    printf("当前时间: %s.%03ld\n", time_str, tv.tv_usec / 1000);  // ‌:ml-citation{ref="3,4" data="citationList"}
}

int main() {
    int sockfd;
    struct sockaddr_un addr;
    FILE *file = NULL;
    char *buffer = NULL;
    long file_size;
    size_t bytes_read;

    // 创建UNIX域数据报套接字
    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        perror("套接字创建失败");
        free(buffer);
        exit(EXIT_FAILURE);
    }

    // 配置目标地址
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    int max_msg_size;
    socklen_t len = sizeof(max_msg_size);
    if (getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &max_msg_size, &len) == 0) {
        printf("最大发送缓冲区: %d 字节\n", max_msg_size);
    }

    int send_buf_size = 1024 * 1024;  // 1MB
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &send_buf_size, sizeof(send_buf_size)) == -1) {
        perror("设置发送缓冲区失败");
    }

    len = sizeof(max_msg_size);
    if (getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &max_msg_size, &len) == 0) {
        printf("最大发送缓冲区: %d 字节\n", max_msg_size);
    }

    printf("当前socket ：%s \n", SOCKET_PATH);

    int i =1;
    while(1){
        for (; i < 251; ++i)
        {
            char filename[100];
            snprintf(filename, sizeof(filename), "yuv/%06d.yuv", i);
            printf("打开文件 %s\n", filename);
            // 打开文件（二进制模式）
            if ((file = fopen(filename, "rb")) == NULL) {
                perror("文件打开失败");
                exit(EXIT_FAILURE);
            }

            // 获取文件大小
            fseek(file, 0, SEEK_END);
            if ((file_size = ftell(file)) == -1) {
                perror("获取文件大小失败");
                fclose(file);
                exit(EXIT_FAILURE);
            }
            rewind(file);

            // 分配内存缓冲区
            if ((buffer = malloc(file_size)) == NULL) {
                perror("内存分配失败");
                fclose(file);
                exit(EXIT_FAILURE);
            }

            // 读取文件内容
            if ((bytes_read = fread(buffer, 1, file_size, file)) != file_size) {
                fprintf(stderr, "文件读取不完整: 期望%ld字节，实际%zu字节\n", file_size, bytes_read);
                free(buffer);
                fclose(file);
                exit(EXIT_FAILURE);
            }
            fclose(file);
            printf("读取数据 %d\n", bytes_read);



            // 发送文件内容
            ssize_t sent_bytes = sendto(
                sockfd,                // 套接字描述符
                buffer,                // 文件数据缓冲区
                file_size,             // 实际文件大小
                0,                     // 无特殊标志
                (struct sockaddr*)&addr,
                sizeof(struct sockaddr_un)
            );

            if (sent_bytes == -1) {
                printf("数据发送失败\n");
            } else {
                printf("成功发送 %zd/%ld 字节到 %s\n", sent_bytes, file_size, filename);
            }

            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 40 * 1000;  // 40毫秒
            select(0, NULL, NULL, NULL, &tv);
            print_time_ms();
        }
    }

    // 清理资源
    free(buffer);
    close(sockfd);
    return EXIT_SUCCESS;
}
