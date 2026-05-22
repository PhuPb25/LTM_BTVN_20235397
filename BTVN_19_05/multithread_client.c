#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define BUF_SIZE 256

void *thread_proc(void *);

int main() {

    // Tao socket
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (client == -1) {
        perror("socket() failed");
        return 1;
    }

    // Khai bao dia chi server
    struct sockaddr_in addr = {0};

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8080);

    // Ket noi den server
    if (connect(client,
                (struct sockaddr *)&addr,
                sizeof(addr))) {

        perror("connect() failed");
        close(client);
        return 1;
    }

    printf("Connected to server on port 8080...\n");

    // Tao thread nhan du lieu
    pthread_t thread_id;

    if (pthread_create(&thread_id,
                       NULL,
                       thread_proc,
                       &client) != 0) {

        perror("pthread_create() failed");
        close(client);
        return 1;
    }

    char buf[BUF_SIZE];

    // Gui du lieu
    while (1) {

        // Nhap du lieu tu ban phim
        if (fgets(buf, sizeof(buf), stdin) == NULL)
            break;

        // Gui du lieu den server
        int ret = send(client,
                       buf,
                       strlen(buf),
                       0);

        if (ret <= 0) {
            printf("Server disconnected.\n");
            break;
        }

        // Neu user nhap exit
        if (strcmp(buf, "exit\n") == 0)
            break;
    }

    // Dong ket noi gui/nhan
    shutdown(client, SHUT_RDWR);

    // Cho thread ket thuc
    pthread_join(thread_id, NULL);

    // Dong socket
    close(client);

    printf("Client closed.\n");

    return 0;
}

// Thread nhan du lieu tu server
void *thread_proc(void *params) {

    int client = *(int *)params;

    char buf[BUF_SIZE];

    while (1) {

        int len = recv(client,
                       buf,
                       sizeof(buf) - 1,
                       0);

        // Server dong ket noi
        if (len <= 0) {
            printf("Disconnected from server.\n");
            break;
        }

        // Them ky tu ket thuc xau
        buf[len] = '\0';

        printf("Received: %s", buf);

        // Neu server gui khong co '\n'
        if (buf[len - 1] != '\n')
            printf("\n");
    }

    return NULL;
}