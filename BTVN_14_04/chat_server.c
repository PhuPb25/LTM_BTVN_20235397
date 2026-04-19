#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <time.h>

#define PORT       8080
#define MAX_CLIENT 100
#define BUF_SIZE   1024

typedef struct
{
    int fd;
    int logged;
    char id[50];
    char name[50];
} Client;

struct pollfd fds[MAX_CLIENT + 1];
Client clients[MAX_CLIENT + 1];

/*------------------------------------------*/
void getTimeStr(char *buf, int size)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    snprintf(buf, size, "[%02d:%02d:%02d]",
             t->tm_hour, t->tm_min, t->tm_sec);
}

/*------------------------------------------*/
void sendToOthers(int sender, const char *msg)
{
    for (int i = 1; i <= MAX_CLIENT; i++)
    {
        if (fds[i].fd > 0 && i != sender && clients[i].logged)
        {
            send(fds[i].fd, msg, strlen(msg), 0);
        }
    }
}

/*------------------------------------------*/
void removeClient(int i)
{
    close(fds[i].fd);
    fds[i].fd = -1;

    clients[i].fd = -1;
    clients[i].logged = 0;
    clients[i].id[0] = 0;
    clients[i].name[0] = 0;
}

/*------------------------------------------*/
int main()
{
    int server_fd;
    struct sockaddr_in addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("listen");
        return 1;
    }

    printf("Chat server running at port %d...\n", PORT);

    /* poll init */
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    for (int i = 1; i <= MAX_CLIENT; i++)
    {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
    }

    while (1)
    {
        poll(fds, MAX_CLIENT + 1, -1);

        /*--------------------------------*/
        /* Client mới kết nối */
        if (fds[0].revents & POLLIN)
        {
            int newfd = accept(server_fd, NULL, NULL);

            for (int i = 1; i <= MAX_CLIENT; i++)
            {
                if (fds[i].fd < 0)
                {
                    fds[i].fd = newfd;

                    clients[i].fd = newfd;
                    clients[i].logged = 0;

                    send(newfd,
                         "Nhap theo dang: client_id: client_name\n",
                         38, 0);
                    break;
                }
            }
        }

        /*--------------------------------*/
        /* Duyệt các client */
        for (int i = 1; i <= MAX_CLIENT; i++)
        {
            if (fds[i].fd < 0) continue;

            if (fds[i].revents & POLLIN)
            {
                char buf[BUF_SIZE];

                int n = recv(fds[i].fd, buf, sizeof(buf) - 1, 0);

                if (n <= 0)
                {
                    printf("%s disconnected\n", clients[i].id);
                    removeClient(i);
                    continue;
                }

                buf[n] = 0;
                buf[strcspn(buf, "\r\n")] = 0;

                /*------------ LOGIN ------------*/
                if (!clients[i].logged)
                {
                    char *p = strchr(buf, ':');

                    if (p == NULL)
                    {
                        send(fds[i].fd,
                             "Sai cu phap!\n",
                             12, 0);
                        continue;
                    }

                    *p = 0;
                    p++;

                    while (*p == ' ') p++;

                    strncpy(clients[i].id, buf,
                            sizeof(clients[i].id) - 1);
                    clients[i].id[49] = 0;

                    strncpy(clients[i].name, p,
                            sizeof(clients[i].name) - 1);
                    clients[i].name[49] = 0;

                    clients[i].logged = 1;

                    send(fds[i].fd,
                         "Dang nhap thanh cong!\n",
                         22, 0);

                    printf("%s connected\n",
                           clients[i].id);
                }

                /*------------ CHAT ------------*/
                else
                {
                    char out[1200];
                    char timebuf[32];

                    getTimeStr(timebuf,
                               sizeof(timebuf));

                    snprintf(out, sizeof(out),
                             "%s %s: %s\n",
                             timebuf,
                             clients[i].id,
                             buf);

                    sendToOthers(i, out);

                    printf("%s", out);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}