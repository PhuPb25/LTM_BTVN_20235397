#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define BUFFER_SIZE 1024

// Hàm xử lý định dạng thời gian dựa trên yêu cầu từ client
void get_formatted_time(const char *format, char *output, size_t max_len) {
    time_t raw_time = time(NULL);
    struct tm *time_info = localtime(&raw_time);
    
    char fmt[64];
    strncpy(fmt, format, sizeof(fmt) - 1);
    fmt[strcspn(fmt, "\r\n")] = 0; // Xóa ký tự xuống dòng

    if (strcmp(fmt, "dd/mm/yyyy") == 0) {
        strftime(output, max_len, "%d/%m/%Y\n", time_info);
    } else if (strcmp(fmt, "dd/mm/yy") == 0) {
        strftime(output, max_len, "%d/%m/%y\n", time_info);
    } else if (strcmp(fmt, "mm/dd/yyyy") == 0) {
        strftime(output, max_len, "%m/%d/%Y\n", time_info);
    } else if (strcmp(fmt, "mm/dd/yy") == 0) {
        strftime(output, max_len, "%m/%d/%y\n", time_info);
    } else {
        // Định dạng mặc định nếu không khớp hoặc không truyền tham số
        strftime(output, max_len, "ERROR: Unsupported format\n", time_info);
    }
}

// Hàm xử lý của mỗi luồng cho từng client kết nối
void *client_handler(void *arg) {
    int client_fd = *(int *)arg;
    free(arg); // Giải phóng bộ nhớ động đã cấp phát ở hàm main

    char buf[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    printf("[Thread %ld] Đang phục vụ client_fd: %d\n", pthread_self(), client_fd);

    while (1) {
        memset(buf, 0, sizeof(buf));
        int len = recv(client_fd, buf, sizeof(buf) - 1, 0);

        if (len <= 0) {
            printf("[Thread %ld] Client %d đã ngắt kết nối.\n", pthread_self(), client_fd);
            break;
        }

        buf[len] = '\0';

        // Xử lý lệnh GET_TIME
        if (strncmp(buf, "GET_TIME", 8) == 0) {
            char *format_ptr = buf + 8;
            while (*format_ptr == ' ') {
                format_ptr++; // Bỏ qua dấu cách thừa
            }
            get_formatted_time(format_ptr, response, sizeof(response));
            send(client_fd, response, strlen(response), 0);
        } else {
            char *err_msg = "Lệnh không hợp lệ. Cú pháp: GET_TIME [format]\n";
            send(client_fd, err_msg, strlen(err_msg), 0);
        }
    }

    close(client_fd);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Sử dụng: %s <Cổng>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener < 0) {
        perror("socket() thất bại");
        return 1;
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind() thất bại");
        close(listener);
        return 1;
    }

    if (listen(listener, 10) < 0) {
        perror("listen() thất bại");
        close(listener);
        return 1;
    }

    printf("Time Server (Multithread) đang chạy trên cổng %d...\n", port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(listener, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept() thất bại");
            continue;
        }

        printf("Kết nối mới từ %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Cấp phát động để tránh lỗi chia sẻ vùng nhớ giữa các Thread (Race Condition)
        int *new_sock = malloc(sizeof(int));
        *new_sock = client_fd;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, client_handler, (void *)new_sock) != 0) {
            perror("Không thể tạo luồng");
            close(client_fd);
            free(new_sock);
        } else {
            // Tách luồng giải phóng tài nguyên tự động khi chạy xong
            pthread_detach(thread_id);
        }
    }

    close(listener);
    return 0;
}