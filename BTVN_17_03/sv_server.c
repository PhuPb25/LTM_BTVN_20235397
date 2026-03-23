#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

struct SinhVien {
    char mssv[20];
    char name[50];
    char dob[20];
    float gpa;
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <PORT> <LOG_FILE>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    char *log_file = argv[2];

    int server = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    bind(server, (struct sockaddr *)&addr, sizeof(addr));
    listen(server, 5);

    printf("Server listening on port %d...\n", port);

    while (1) {
        int client = accept(server, (struct sockaddr *)&client_addr, &addr_len);

        struct SinhVien sv;
        int n = recv(client, &sv, sizeof(sv), 0);

        if (n > 0) {
            // Lấy IP
            char *ip = inet_ntoa(client_addr.sin_addr);

            // Lấy thời gian
            time_t now = time(NULL);
            struct tm *t = localtime(&now);

            char time_str[50];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

            // In ra màn hình
            printf("%s %s %s %s %s %.2f\n", ip, time_str,
                   sv.mssv, sv.name, sv.dob, sv.gpa);

            // Ghi file
            FILE *f = fopen(log_file, "a");
            if (f != NULL) {
                fprintf(f, "%s %s %s %s %s %.2f\n",
                        ip, time_str,
                        sv.mssv, sv.name, sv.dob, sv.gpa);
                fclose(f);
            }
        }

        close(client);
    }

    close(server);
    return 0;
}