#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Cấu trúc quản lý một cặp chat (Phòng chat đôi)
typedef struct {
    int client1_fd;
    int client2_fd;
    int is_active;
} ChatPair;

// Các biến toàn cục phục vụ hàng đợi kết nối và đồng bộ luồng
int waiting_client = -1; // Client đang cô đơn ngồi đợi ghép cặp
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

// Cấu trúc truyền tham số vào luồng xử lý riêng cho từng client
typedef struct {
    int my_fd;
    int partner_fd;
    ChatPair *pair_info;
} ThreadArgs;

// Luồng chuyển tiếp tin nhắn từ Client này sang Client kia
void *forward_handler(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    int my_fd = args->my_fd;
    int partner_fd = args->partner_fd;
    ChatPair *pair = args->pair_info;
    free(args); // Giải phóng bộ nhớ tham số luồng

    char buf[BUFFER_SIZE];

    while (1) {
        memset(buf, 0, sizeof(buf));
        int len = recv(my_fd, buf, sizeof(buf) - 1, 0);

        // Nếu client ngắt kết nối hoặc lỗi
        if (len <= 0) {
            printf("[Server] Client fd %d da ngat ket noi.\n", my_fd);
            break;
        }

        buf[len] = '\0';

        // Khóa mutex để kiểm tra và chuyển tiếp dữ liệu một cách an toàn
        pthread_mutex_lock(&queue_mutex);
        if (pair->is_active) {
            // Chuyển tiếp tin nhắn sang cho partner
            send(partner_fd, buf, len, 0);
        }
        pthread_mutex_unlock(&queue_mutex);
    }

    // Xử lý ngắt kết nối dây chuyền: Nếu mình sập, sập luôn cả phòng và bạn chat
    pthread_mutex_lock(&queue_mutex);
    if (pair->is_active) {
        pair->is_active = 0;
        char *msg = "[Server] Ban chat cua ban da roi phong. Ngat ket noi!\n";
        send(partner_fd, msg, strlen(msg), 0);
        close(partner_fd); // Đóng socket của đối phương
        printf("[Server] Da tu dong dong socket cua partner fd %d\n", partner_fd);
    }
    pthread_mutex_unlock(&queue_mutex);

    close(my_fd);
    pthread_exit(NULL);
}

// Luồng trung gian làm nhiệm vụ "Bà mai" để ghép đôi
void *matchmaker_thread(void *arg) {
    while (1) {
        pthread_mutex_lock(&queue_mutex);

        // Nếu hàng đợi chưa đủ 2 người (chỉ có -1 hoặc có 1 người đang đợi), luồng sẽ ngủ
        while (waiting_client == -1) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }

        // Đã có 1 người đợi sẵn từ trước, và luồng chính vừa báo hiệu có người thứ 2
        int c1 = waiting_client;
        pthread_mutex_unlock(&queue_mutex);

        // Đợi luồng chính gán client thứ hai vào một biến cục bộ tạm thời (xử lý tiếp ở vòng lặp nhận)
        // Để đơn giản và an toàn, cấu trúc điều phối trực tiếp từ vòng lặp main dưới đây sẽ tối ưu hơn.
    }
    return NULL;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener < 0) {
        perror("socket() that bai");
        return 1;
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
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

    printf("Pair Chat Server (Đa luồng) đang chạy trên cổng %d...\n", PORT);
    printf("Dang doi cac client ket noi de ghep cap...\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(listener, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_fd < 0) {
            continue;
        }

        pthread_mutex_lock(&queue_mutex);

        // KIỂM TRA HÀNG ĐỢI GHÉP CẶP
        if (waiting_client == -1) {
            // Hàng đợi trống -> Client này là người thứ 1, bắt ngồi đợi
            waiting_client = client_fd;
            char *welcome = "[Server] Chao mung ban! Vui long doi ban chat ket noi...\n";
            send(client_fd, welcome, strlen(welcome), 0);
            printf("[Server] Client fd %d dang vao hang doi, doi ghep cap.\n", client_fd);
            pthread_mutex_unlock(&queue_mutex);
        } 
        else {
            // Đã có 1 người đợi sẵn -> Client mới này là người thứ 2 -> THÀNH CẶP!
            int client1 = waiting_client;
            int client2 = client_fd;
            waiting_client = -1; // Reset hàng đợi về trống để người sau vào đợi tiếp

            printf("[Server] Ghep cap thanh cong: fd %d <=> fd %d\n", client1, client2);

            // Thông báo cho cả 2 client biết đã tìm thấy nhau
            char *start_msg = "[Server] Da ghep cap thanh cong! Bat dau chat nhóm 2 nguoi.\n";
            send(client1, start_msg, strlen(start_msg), 0);
            send(client2, start_msg, strlen(start_msg), 0);

            // Cấp phát động một cấu trúc quản lý cặp chat này
            ChatPair *new_pair = malloc(sizeof(ChatPair));
            new_pair->client1_fd = client1;
            new_pair->client2_fd = client2;
            new_pair->is_active = 1;

            pthread_mutex_unlock(&queue_mutex);

            // TẠO 2 LUỒNG RIÊNG BIỆT để phục vụ việc nhận tin song song từ mỗi người
            pthread_t t1, t2;

            ThreadArgs *args1 = malloc(sizeof(ThreadArgs));
            args1->my_fd = client1;
            args1->partner_fd = client2;
            args1->pair_info = new_pair;

            ThreadArgs *args2 = malloc(sizeof(ThreadArgs));
            args2->my_fd = client2;
            args2->partner_fd = client1;
            args2->pair_info = new_pair;

            pthread_create(&t1, NULL, forward_handler, (void *)args1);
            pthread_create(&t2, NULL, forward_handler, (void *)args2);

            // Tách luồng tự do giải phóng
            pthread_detach(t1);
            pthread_detach(t2);
        }
    }

    close(listener);
    return 0;
}