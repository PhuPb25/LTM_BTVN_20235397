//===== Trần Anh Phú - 20235397 ========
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <IP> <PORT>\n", argv[0]);
        return 1;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    int client = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_port = htons(port);

    if (connect(client, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect failed");
        return 1;
    }

    // ===== Lấy thư mục hiện tại =====
    char cwd[256];
    getcwd(cwd, sizeof(cwd));

    // Buffer gửi
    char data[4096];
    strcpy(data, cwd);

    // ===== Đọc file =====
    DIR *dir = opendir(".");
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        struct stat st;

        if (stat(entry->d_name, &st) == 0 && S_ISREG(st.st_mode)) {
            char temp[512];
            snprintf(temp, sizeof(temp), "|%s:%ld", entry->d_name, st.st_size);
            strcat(data, temp);
        }
    }

    closedir(dir);

    // ===== Gửi =====
    send(client, data, strlen(data), 0);

    close(client);
    return 0;
}
