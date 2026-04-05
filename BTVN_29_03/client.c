#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    // 🔥 check tham số
    if (argc != 3) {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        return 1;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // 1. Tạo socket
    sock = socket(AF_INET, SOCK_STREAM, 0);

    // 2. Cấu hình server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    // 3. Kết nối server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Ket noi that bai\n");
        return -1;
    }

    printf("Da ket noi server %s:%d!\n", ip, port);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);

        // 4. Nhận từ server
        int bytes = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) {
            printf("Server dong ket noi\n");
            break;
        }

        printf("%s", buffer);

        // 5. Nhập từ bàn phím
        fgets(buffer, BUFFER_SIZE, stdin);

        // 6. Gửi server
        send(sock, buffer, strlen(buffer), 0);
    }

    close(sock);
    return 0;
}