#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    int client = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(9000);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(client, (struct sockaddr *)&server, sizeof(server));

    char buf[1024];

    while (1) {
        printf("Enter text ('exit' to quit): ");
        fgets(buf, sizeof(buf), stdin);

        //Xóa ký tự '\n'
        buf[strcspn(buf, "\n")] = 0;

        //Nếu nhập exit → thoát
        if (strcmp(buf, "exit") == 0) {
            break;
        }

        send(client, buf, strlen(buf), 0);
    }

    close(client);
    return 0;
}