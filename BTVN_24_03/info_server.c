#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <PORT>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);

    int server = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    bind(server, (struct sockaddr *)&addr, sizeof(addr));
    listen(server, 5);

    printf("Server listening...\n");

    int client = accept(server, NULL, NULL);

    char buf[4096];
    int n = recv(client, buf, sizeof(buf) - 1, 0);
    buf[n] = 0;

    // ===== Tách dữ liệu =====
    char *token = strtok(buf, "|");

    printf("Folder: %s\n", token);

    while ((token = strtok(NULL, "|")) != NULL) {
        char name[256];
        long size;

        sscanf(token, "%[^:]:%ld", name, &size);

        printf("File: %s - Size: %ld bytes\n", name, size);
    }

    close(client);
    close(server);
    return 0;
}