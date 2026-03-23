#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

struct SinhVien {
    char mssv[20];
    char name[50];
    char dob[20];
    float gpa;
};

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

    struct SinhVien sv;

    printf("Nhap MSSV: ");
    fgets(sv.mssv, sizeof(sv.mssv), stdin);
    sv.mssv[strcspn(sv.mssv, "\n")] = 0;

    printf("Nhap ho ten: ");
    fgets(sv.name, sizeof(sv.name), stdin);
    sv.name[strcspn(sv.name, "\n")] = 0;

    printf("Nhap ngay sinh: ");
    fgets(sv.dob, sizeof(sv.dob), stdin);
    sv.dob[strcspn(sv.dob, "\n")] = 0;

    printf("Nhap GPA: ");
    scanf("%f", &sv.gpa);

    // Gửi struct
    send(client, &sv, sizeof(sv), 0);

    close(client);
    return 0;
}