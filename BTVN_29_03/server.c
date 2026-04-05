#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024

typedef struct {
    int state;
    char name[100];
    char mssv[50];
} ClientState;

void generateEmail(char *fullName, char *mssv, char *email) {
    char nameCopy[100];
    strcpy(nameCopy, fullName);

    nameCopy[strcspn(nameCopy, "\n")] = 0;
    mssv[strcspn(mssv, "\n")] = 0;

    char *words[10];
    int count = 0;

    char *token = strtok(nameCopy, " ");
    while (token != NULL) {
        words[count++] = token;
        token = strtok(NULL, " ");
    }

    char lastName[50];
    strcpy(lastName, words[count - 1]);
    lastName[0] = toupper(lastName[0]);

    char initials[10] = "";
    for (int i = 0; i < count - 1; i++) {
        char c = toupper(words[i][0]);
        strncat(initials, &c, 1);
    }

    char idPart[50];
    strcpy(idPart, mssv + 2);

    sprintf(email, "%s.%s%s@sis.hust.edu.vn", lastName, initials, idPart);
}

int main(int argc, char *argv[]) {
    //check tham số
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);

    int server_fd, new_socket, client_sockets[MAX_CLIENTS];
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    fd_set readfds;
    ClientState states[MAX_CLIENTS];
    char buffer[BUFFER_SIZE];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
        states[i].state = 0;
    }

    // tạo socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    printf("Server dang chay tai port %d...\n", port);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);

        int max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];

            if (sd > 0)
                FD_SET(sd, &readfds);

            if (sd > max_sd)
                max_sd = sd;
        }

        select(max_sd + 1, &readfds, NULL, NULL, NULL);

        // client mới
        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);

            printf("Co client moi ket noi\n");

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    states[i].state = 0;

                    send(new_socket, "Nhap ho ten: ", 15, 0);
                    break;
                }
            }
        }

        // xử lý client
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) {
                int valread = recv(sd, buffer, BUFFER_SIZE, 0);

                if (valread == 0) {
                    close(sd);
                    client_sockets[i] = 0;
                    printf("Client ngat ket noi\n");
                } else {
                    buffer[valread] = '\0';

                    if (states[i].state == 0) {
                        strcpy(states[i].name, buffer);
                        send(sd, "Nhap MSSV: ", 12, 0);
                        states[i].state = 1;
                    }
                    else if (states[i].state == 1) {
                        strcpy(states[i].mssv, buffer);

                        char email[100];
                        generateEmail(states[i].name, states[i].mssv, email);

                        strcat(email, "\n");
                        send(sd, email, strlen(email), 0); 

                        states[i].state = 0;
                        send(sd, "\nNhap ho ten: ", 16, 0);
                    }
                }
            }
        }
    }

    return 0;
}