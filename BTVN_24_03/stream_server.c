#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PATTERN "0123456789"
#define LEN 10

int count_pattern(char *str) {
    int count = 0;
    for (int i = 0; str[i]; i++) {
        if (strncmp(&str[i], PATTERN, LEN) == 0) {
            count++;
        }
    }
    return count;
}

int main() {
    int server = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server, (struct sockaddr *)&addr, sizeof(addr));
    listen(server, 5);

    printf("Server listening...\n");

    int client = accept(server, NULL, NULL);

    char buf[1024];
    char prev[LEN] = "";   // lưu phần dư
    int total = 0;

    while (1) {
        int n = recv(client, buf, sizeof(buf) - 1, 0);
        if (n <= 0) break;

        buf[n] = 0;

        // nối prev + buf
        char combined[2048];
        strcpy(combined, prev);
        strcat(combined, buf);

        // đếm
        int c = count_pattern(combined);
        total += c;

        printf("Received: %s\n", buf);
        printf("Total count: %d\n\n", total);

        // giữ lại 9 ký tự cuối
        int len = strlen(combined);
        if (len >= LEN - 1) {
            strncpy(prev, combined + len - (LEN - 1), LEN - 1);
            prev[LEN - 1] = 0;
        } else {
            strcpy(prev, combined);
        }
    }

    close(client);
    close(server);
    return 0;
}