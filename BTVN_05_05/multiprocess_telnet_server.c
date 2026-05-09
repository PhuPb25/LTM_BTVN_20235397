#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

#define PORT 9001
#define BUFFER_SIZE 1024

void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int check_login(char *user, char *pass) {
    FILE *f = fopen("users.txt", "r");
    if (f == NULL) {
        perror("fopen");
        return 0;
    }

    char u[100], p[100];

    while (fscanf(f, "%s %s", u, p) != EOF) {
        if (strcmp(user, u) == 0 &&
            strcmp(pass, p) == 0) {
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}

void trim_newline(char *s) {
    s[strcspn(s, "\r\n")] = 0;
}

void handle_client(int client) {
    char buf[BUFFER_SIZE];
    char username[100];
    char password[100];

    send(client, "Username: ", 10, 0);

    int len = recv(client, username, sizeof(username)-1, 0);
    if (len <= 0) return;
    username[len] = 0;
    trim_newline(username);

    send(client, "Password: ", 10, 0);

    len = recv(client, password, sizeof(password)-1, 0);
    if (len <= 0) return;
    password[len] = 0;
    trim_newline(password);

    if (!check_login(username, password)) {
        send(client, "Login failed!\n", 14, 0);
        close(client);
        return;
    }

    send(client, "Login success!\n", 15, 0);

    while (1) {
        send(client, "> ", 2, 0);

        len = recv(client, buf, sizeof(buf)-1, 0);

        if (len <= 0)
            break;

        buf[len] = 0;
        trim_newline(buf);

        if (strcmp(buf, "exit") == 0)
            break;

        // Chạy lệnh shell
        FILE *fp = popen(buf, "r");

        if (fp == NULL) {
            send(client, "Command error!\n", 15, 0);
            continue;
        }

        char output[BUFFER_SIZE];

        while (fgets(output, sizeof(output), fp) != NULL) {
            send(client, output, strlen(output), 0);
        }

        pclose(fp);
    }

    close(client);
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

    printf("Telnet server listening on port %d...\n", PORT);

    while (1) {
        int client = accept(listener, NULL, NULL);

        if (client < 0)
            continue;

        printf("New client connected: %d\n", client);

        pid_t pid = fork();

        if (pid == 0) {
            // Process con
            close(listener);

            handle_client(client);

            printf("Client disconnected: %d\n", client);

            exit(0);
        }

        // Process cha
        close(client);
    }

    close(listener);
    return 0;
}