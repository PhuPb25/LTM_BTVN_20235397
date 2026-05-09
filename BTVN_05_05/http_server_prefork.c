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
#define BUFFER_SIZE 4096
#define WORKER_COUNT 4

void handle_client(int client) {
    char buf[BUFFER_SIZE];

    int len = recv(client, buf, sizeof(buf)-1, 0);

    if (len <= 0) {
        close(client);
        return;
    }

    buf[len] = 0;

    printf("========== REQUEST ==========\n");
    printf("%s\n", buf);

    // HTTP response đơn giản
    char response[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<html>"
        "<head><title>Prefork HTTP Server</title></head>"
        "<body>"
        "<h1>Hello from Prefork Server!</h1>"
        "<p>HTTP Server using preforking.</p>"
        "</body>"
        "</html>";

    send(client, response, strlen(response), 0);

    close(client);
}

void worker_process(int listener) {
    while (1) {
        int client = accept(listener, NULL, NULL);

        if (client < 0)
            continue;

        printf("[PID %d] Client connected\n", getpid());

        handle_client(client);
    }
}

void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listener < 0) {
        perror("socket");
        return 1;
    }

    // Cho phép reuse port
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

    if (listen(listener, 10) < 0) {
        perror("listen");
        close(listener);
        return 1;
    }

    signal(SIGCHLD, sigchld_handler);

    printf("HTTP Prefork Server listening on port %d...\n", PORT);

    // Tạo sẵn worker process
    for (int i = 0; i < WORKER_COUNT; i++) {

        pid_t pid = fork();

        if (pid == 0) {
            // Worker process
            worker_process(listener);
            exit(0);
        }
    }

    // Process cha chờ
    while (1) {
        pause();
    }

    close(listener);

    return 0;
}