#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define THREAD_COUNT 2 // Số lượng luồng tạo sẵn (Thread Pool)

// Hàm xử lý Request và phản hồi HTTP Response đơn giản cho Client
void handle_client(int client) {
    char buf[BUFFER_SIZE];

    // Nhận dữ liệu Request từ Client (Trình duyệt hoặc công cụ như curl/nc)
    int len = recv(client, buf, sizeof(buf) - 1, 0);
    if (len <= 0) {
        close(client);
        return;
    }

    buf[len] = 0;

    // In Request ra màn hình của Server để kiểm tra
    printf("========== REQUEST ON THREAD %ld ==========\n", pthread_self());
    printf("%s\n", buf);

    // Chuỗi phản hồi chuẩn HTTP/1.1 kèm nội dung HTML
    char response[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<html>"
        "<head><title>Pre-threading HTTP Server</title></head>"
        "<body>"
        "<h1>Hello from Pre-threading HTTP Server!</h1>"
        "<p>Ung dung HTTP Server su dung ky thuat tao san luong (Pre-threading).</p>"
        "</body>"
        "</html>";

    // Gửi Response về cho Client
    send(client, response, strlen(response), 0);

    // Đóng kết nối (HTTP Stateless)
    close(client);
}

// Hàm thực thi của luồng Worker
void *worker_thread(void *arg) {
    int listener = *(int *)arg;

    printf("[Luon %ld] Da khoi tao va dang doi ket noi...\n", pthread_self());

    while (1) {
        // Nhiều luồng cùng gọi accept trên một socket listener chung.
        // Hệ điều hành sẽ đảm bảo tính đồng bộ (Mutual Exclusion) để chỉ 1 luồng nhận được kết nối.
        int client = accept(listener, NULL, NULL);

        if (client < 0) {
            continue;
        }

        printf("[Luon %ld] Da chap nhan ket noi thanh cong!\n", pthread_self());

        // Xử lý Request của client vừa accept được
        handle_client(client);
    }

    return NULL;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener < 0) {
        perror("socket() that bai");
        return 1;
    }

    // Cho phép tái sử dụng địa chỉ/cổng để tránh lỗi "Address already in use"
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind() that bai");
        close(listener);
        return 1;
    }

    if (listen(listener, 10) < 0) {
        perror("listen() that bai");
        close(listener);
        return 1;
    }

    printf("HTTP Pre-threading Server dang lang nghe tai cong %d...\n", PORT);

    // Mảng lưu ID của các luồng
    pthread_t workers[THREAD_COUNT];

    // Tạo sẵn tập hợp luồng (Thread Pool)
    for (int i = 0; i < THREAD_COUNT; i++) {
        if (pthread_create(&workers[i], NULL, worker_thread, (void *)&listener) != 0) {
            perror("Khong the tao luon worker");
            close(listener);
            return 1;
        }
    }

    // Luồng chính (main thread) đợi các luồng worker hoàn thành (thực tế vòng lặp worker là vô hạn)
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(workers[i], NULL);
    }

    close(listener);
    return 0;
}