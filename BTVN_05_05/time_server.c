#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>

#define PORT 9001
#define BUFFER_SIZE 1024

void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void trim_newline(char *s) {
    s[strcspn(s, "\r\n")] = 0;
}

int main() {

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listener < 0) {
        perror("socket");
        return 1;
    }

    setsockopt(listener,
               SOL_SOCKET,
               SO_REUSEADDR,
               &(int){1},
               sizeof(int));

    struct sockaddr_in addr = {0};

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(listener,
             (struct sockaddr*)&addr,
             sizeof(addr)) < 0) {

        perror("bind");
        close(listener);
        return 1;
    }

    if (listen(listener, 5) < 0) {
        perror("listen");
        close(listener);
        return 1;
    }

    signal(SIGCHLD, sigchld_handler);

    printf("Time server listening on port %d...\n", PORT);

    while (1) {

        int client = accept(listener, NULL, NULL);

        if (client < 0)
            continue;

        printf("New client connected: %d\n", client);

        pid_t pid = fork();

        if (pid == 0) {

            close(listener);

            char buf[BUFFER_SIZE];

            while (1) {

                int len = recv(client,
                               buf,
                               sizeof(buf)-1,
                               0);

                if (len <= 0)
                    break;

                buf[len] = 0;

                trim_newline(buf);

                printf("Client %d: %s\n", client, buf);

                char cmd[100];
                char format[100];

                int n = sscanf(buf,
                               "%s %s",
                               cmd,
                               format);

                // Kiem tra cu phap
                if (n != 2 ||
                    strcmp(cmd, "GET_TIME") != 0) {

                    send(client,
                         "ERROR: Invalid command\n",
                         23,
                         0);

                    continue;
                }

                time_t now = time(NULL);

                struct tm *t = localtime(&now);

                char response[100];

                // Xu ly format
                if (strcmp(format,
                           "dd/mm/yyyy") == 0) {

                    strftime(response,
                             sizeof(response),
                             "%d/%m/%Y",
                             t);
                }
                else if (strcmp(format,
                                "dd/mm/yy") == 0) {

                    strftime(response,
                             sizeof(response),
                             "%d/%m/%y",
                             t);
                }
                else if (strcmp(format,
                                "mm/dd/yyyy") == 0) {

                    strftime(response,
                             sizeof(response),
                             "%m/%d/%Y",
                             t);
                }
                else if (strcmp(format,
                                "mm/dd/yy") == 0) {

                    strftime(response,
                             sizeof(response),
                             "%m/%d/%y",
                             t);
                }
                else {

                    send(client,
                         "ERROR: Unsupported format\n",
                         28,
                         0);

                    continue;
                }

                strcat(response, "\n");

                send(client,
                     response,
                     strlen(response),
                     0);
            }

            printf("Client disconnected: %d\n", client);

            close(client);

            exit(0);
        }

        close(client);
    }

    close(listener);

    return 0;
}