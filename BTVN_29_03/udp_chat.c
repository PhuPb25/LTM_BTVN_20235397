#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    // kiểm tra tham số
    if (argc != 4) {
        printf("Usage: %s <port_s> <ip_d> <port_d>\n", argv[0]);
        return 1;
    }

    int port_s = atoi(argv[1]);   // port nhận
    char *ip_d = argv[2];         // IP đích
    int port_d = atoi(argv[3]);   // port đích

    int sock;
    struct sockaddr_in local_addr, dest_addr, sender_addr;
    socklen_t sender_len = sizeof(sender_addr);

    char buffer[BUFFER_SIZE];

    fd_set readfds;

    // tạo socket UDP
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket error");
        return 1;
    }

    // bind địa chỉ local
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(port_s);

    if (bind(sock, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        perror("Bind error");
        return 1;
    }

    // cấu hình địa chỉ đích
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port_d);
    dest_addr.sin_addr.s_addr = inet_addr(ip_d);

    printf("UDP Chat dang chay...\n");
    printf("Nhan tai port: %d\n", port_s);
    printf("Gui den: %s:%d\n", ip_d, port_d);
    printf("Nhap /exit de thoat\n\n");

    while (1) {
        FD_ZERO(&readfds);

        // theo dõi bàn phím và socket
        FD_SET(0, &readfds);       // stdin
        FD_SET(sock, &readfds);    // socket

        int maxfd = sock;

        // non-blocking với select
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("Select error");
            break;
        }

        // ========================
        //  GỬI DỮ LIỆU
        // ========================
        if (FD_ISSET(0, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            fgets(buffer, BUFFER_SIZE, stdin);

            // thoát
            if (strncmp(buffer, "/exit", 5) == 0) {
                printf("Thoat chuong trinh...\n");
                break;
            }

            sendto(sock, buffer, strlen(buffer), 0,
                   (struct sockaddr*)&dest_addr, sizeof(dest_addr));
        }

        // ========================
        // NHẬN DỮ LIỆU
        // ========================
        if (FD_ISSET(sock, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);

            int n = recvfrom(sock, buffer, BUFFER_SIZE - 1, 0,
                             (struct sockaddr*)&sender_addr, &sender_len);

            if (n > 0) {
                buffer[n] = '\0';

                printf("\n[%s:%d]: %s",
                       inet_ntoa(sender_addr.sin_addr),
                       ntohs(sender_addr.sin_port),
                       buffer);

                printf(">> "); // gợi ý nhập tiếp
                fflush(stdout);
            }
        }
    }

    close(sock);
    return 0;
}