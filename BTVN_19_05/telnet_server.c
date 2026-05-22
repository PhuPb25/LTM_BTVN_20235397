#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUF_SIZE 1024

int send_str(int fd, const char *msg) {
    return send(fd, msg, strlen(msg), 0);
}

int check_login(char *user, char *pass) {

    FILE *f = fopen("users.txt", "r");

    if (!f)
        return 0;

    char u[50], p[50];

    while (fscanf(f, "%s %s", u, p) == 2) {

        if (strcmp(user, u) == 0 &&
            strcmp(pass, p) == 0) {

            fclose(f);
            return 1;
        }
    }

    fclose(f);

    return 0;
}

void execute_command(int fd, char *cmd) {

    FILE *fp = popen(cmd, "r");

    if (!fp) {
        send_str(fd, "Command failed\n");
        return;
    }

    char buf[BUF_SIZE];

    while (fgets(buf, sizeof(buf), fp)) {
        send_str(fd, buf);
    }

    pclose(fp);
}

void trim(char *s) {
    s[strcspn(s, "\r\n")] = 0;
}

void *client_thread(void *arg) {

    int client = *(int *)arg;
    free(arg);

    char user[50];
    char pass[50];
    char buf[BUF_SIZE];

    send_str(client, "Username: ");

    int ret = recv(client, user, sizeof(user), 0);

    if (ret <= 0)
        goto END;

    user[ret] = 0;
    trim(user);

    send_str(client, "Password: ");

    ret = recv(client, pass, sizeof(pass), 0);

    if (ret <= 0)
        goto END;

    pass[ret] = 0;
    trim(pass);

    if (!check_login(user, pass)) {

        send_str(client, "Login failed\n");

        goto END;
    }

    send_str(client, "Login successful\n");

    while (1) {

        send_str(client, "> ");

        ret = recv(client,
                   buf,
                   sizeof(buf) - 1,
                   0);

        if (ret <= 0)
            break;

        buf[ret] = 0;

        trim(buf);

        if (strcmp(buf, "exit") == 0)
            break;

        execute_command(client, buf);
    }

END:

    printf("Client disconnected\n");

    close(client);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int listener = socket(AF_INET,
                          SOCK_STREAM,
                          IPPROTO_TCP);

    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listener,
         (struct sockaddr *)&addr,
         sizeof(addr));

    listen(listener, 10);

    printf("Telnet server listening...\n");

    while (1) {

        int client =
            accept(listener, NULL, NULL);

        if (client < 0)
            continue;

        pthread_t tid;

        int *pclient = malloc(sizeof(int));
        *pclient = client;

        pthread_create(&tid,
                       NULL,
                       client_thread,
                       pclient);

        pthread_detach(tid);
    }

    close(listener);

    return 0;
}