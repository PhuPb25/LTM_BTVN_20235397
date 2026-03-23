#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    // Kiểm tra tham số dòng lệnh
    if (argc != 3) {
        printf("Usage: %s <IP> <PORT>\n", argv[0]);
        return 1;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    // Tạo socket TCP
    int client = socket(AF_INET, SOCK_STREAM, 0);

    // Khai báo địa chỉ server
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    // Kết nối đến server
    if (connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        return 1;
    }

    printf("Connected to %s:%d\n", ip, port);
    
    char buf[256];
    int n = recv(client, buf, sizeof(buf), 0);
    if (n > 0) {
        buf[n] = 0;
        printf("Server says:\n%s\n", buf);
    }

    // Gửi dữ liệu từ bàn phím
    while (1) {
        printf("> ");
        fgets(buf, sizeof(buf), stdin);

        // xóa '\n'
        buf[strcspn(buf, "\n")] = 0;

        if (strcmp(buf, "exit") == 0) break;

        send(client, buf, strlen(buf), 0);
    }

    close(client);
    return 0;
}