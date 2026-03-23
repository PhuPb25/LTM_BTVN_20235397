//===== Trần Anh Phú - 20235397 ========
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <PORT> <WELCOME_FILE> <OUTPUT_FILE>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    char *welcome_file = argv[2];
    char *output_file = argv[3];

    // Tạo socket
    int server = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    bind(server, (struct sockaddr *)&addr, sizeof(addr));
    listen(server, 5);

    printf("Server listening on port %d...\n", port);

    while (1) {
        int client = accept(server, NULL, NULL);
        printf("Client connected\n");

        // ===== Gửi file chào =====
        FILE *f = fopen(welcome_file, "r");
        if (f != NULL) {
            char buf[256];
            int n;
            while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
                send(client, buf, n, 0);
            }
            fclose(f);
        }

        // ===== Ghi dữ liệu client =====
        FILE *out = fopen(output_file, "a");
        if (out == NULL) {
            perror("Cannot open output file");
            close(client);
            continue;
        }

        char buf[256];
        int n;
        while ((n = recv(client, buf, sizeof(buf), 0)) > 0) {
            buf[n] = 0;
            
            printf("Client: %s\n", buf);

            fprintf(out, "%s\n", buf);
        }

        fclose(out);
        close(client);

        printf("Client disconnected\n");
    }

    close(server);
    return 0;
}