#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 2048

// Luồng liên tục lắng nghe từ Server
void* receive_thread(void *socket_desc) {
    int sock = *(int*)socket_desc;
    char buffer[BUFFER_SIZE];
    
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            printf("\n[Hệ thống] Đã mất kết nối tới Server.\n");
            exit(0);
        }
        // In trực tiếp thông báo từ Server
        printf("\n%s> ", buffer);
        fflush(stdout);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Sử dụng: %s <IP_Server> <Cổng>\n", argv[0]);
        return 1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Lỗi kết nối");
        return 1;
    }
    
    printf("[Hệ thống] Kết nối thành công tới %s:%s\n", argv[1], argv[2]);
    printf("[Hệ thống] Lệnh hỗ trợ: JOIN, MSG, PMSG, OP, KICK, TOPIC, QUIT\n");
    printf("==================================================\n");
    printf("          CÁC LỆNH HỖ TRỢ TRONG PHÒNG CHAT        \n");
    printf("==================================================\n");
    printf(" 1. JOIN <nickname>       - Tham gia phòng chat\n");
    printf(" 2. MSG <tin_nhắn>        - Gửi tin nhắn cho cả phòng\n");
    printf(" 3. PMSG <user> <tin_nhắn>- Gửi tin nhắn cá nhân\n");
    printf(" 4. OP <user>             - Chuyển quyền chủ phòng\n");
    printf(" 5. KICK <user>           - Đuổi người dùng khỏi phòng\n");
    printf(" 6. TOPIC <chủ_đề>        - Thiết lập chủ đề phòng\n");
    printf(" 7. QUIT                  - Thoát khỏi phòng chat\n");
    printf("==================================================\n");

    // Khởi tạo luồng nhận dữ liệu
    pthread_t recv_th;
    pthread_create(&recv_th, NULL, receive_thread, (void*)&sock);
    // pthread_detach để luồng tự dọn dẹp bộ nhớ
    pthread_detach(recv_th); 

    char message[BUFFER_SIZE];
    while (1) {
        printf("> ");
        fflush(stdout);
        fgets(message, BUFFER_SIZE, stdin);
        
        // Gửi lệnh lên Server (bảo toàn ký tự \n)
        send(sock, message, strlen(message), 0);
        
        // Nếu gõ QUIT thì kết thúc vòng lặp để đóng chương trình
        if (strncmp(message, "QUIT", 4) == 0) {
            // Chờ một khoảnh khắc nhỏ để đọc nốt phản hồi '100 OK'
            usleep(100000); 
            break;
        }
    }

    close(sock);
    printf("[Hệ thống] Đã thoát chương trình.\n");
    return 0;
}