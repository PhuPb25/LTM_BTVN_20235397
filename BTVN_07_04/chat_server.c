#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <time.h>

#define MAX_CLIENTS 1024
#define BUF_SIZE 512

typedef struct {
    int sock;
    int logged_in;
    char id[50];
    char name[50];
} Client;

Client clients[MAX_CLIENTS];

void removeClient(int i) {
    close(clients[i].sock);
    clients[i].sock = 0;
    clients[i].logged_in = 0;
}

void broadcast(int sender, char *msg) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock != 0 && i != sender && clients[i].logged_in) {
            send(clients[i].sock, msg, strlen(msg), 0);
        }
    }
}

//Lấy thời gian hiện tại
void getTimeStr(char *buf) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buf, 64, "%d/%m/%Y %H:%M:%S", t);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        printf("Invalid port\n");
        return 1;
    }
    int listener = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 10);

    printf("Server listening on port %d...\n", port);

    fd_set master, readfds;
    FD_ZERO(&master);
    FD_SET(listener, &master);

    int maxfd = listener;

    char buf[BUF_SIZE];

    while (1) {
        readfds = master;

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        for (int i = 0; i <= maxfd; i++) {
            if (!FD_ISSET(i, &readfds)) continue;

            //Client mới
            if (i == listener) {
                int client = accept(listener, NULL, NULL);
                if (client < 0) continue;

                FD_SET(client, &master);
                if (client > maxfd) maxfd = client;

                clients[client].sock = client;
                clients[client].logged_in = 0;

                char *msg = "Nhap theo format: client_id: client_name\n";
                send(client, msg, strlen(msg), 0);

                printf("New client: %d\n", client);
            }

            //Client gửi dữ liệu
            else {
                int ret = recv(i, buf, BUF_SIZE - 1, 0);
                if (ret <= 0) {
                    printf("Client %d disconnected\n", i);
                    removeClient(i);
                    FD_CLR(i, &master);
                    continue;
                }

                buf[ret] = '\0';

                //Nếu chưa login
                if (!clients[i].logged_in) {
                    char *colon = strchr(buf, ':');
                    if (!colon) {
                        char *msg = "Sai format. Dung: id: name\n";
                        send(i, msg, strlen(msg), 0);
                        continue;
                    }

                    *colon = '\0';
                    char *id = buf;
                    char *name = colon + 1;

                    while (*name == ' ') name++;

                    strcpy(clients[i].id, id);
                    strcpy(clients[i].name, name);
                    clients[i].logged_in = 1;

                    char msg[BUF_SIZE];
                    snprintf(msg, sizeof(msg), "Welcome %s (%s)\n", id, name);
                    send(i, msg, strlen(msg), 0);

                    printf("Client %d logged in as %s\n", i, id);
                }

                //Đã login → broadcast
                else {
                    char timebuf[64];
                    getTimeStr(timebuf);

                    char out[BUF_SIZE];
                    snprintf(out, sizeof(out), "[%s] %.20s: %.400s",
                            timebuf,
                            clients[i].id,
                            buf);

                    printf("%s", out);
                    broadcast(i, out);
                }
            }
        }
    }

    close(listener);
    return 0;
}