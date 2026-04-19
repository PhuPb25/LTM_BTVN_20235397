#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <poll.h>

#define PORT        8080
#define MAX_CLIENT  100
#define BUF_SIZE    1024

typedef struct
{
    int fd;
    int state;          // 0:user, 1:pass, 2:login ok
    char user[50];
    char pass[50];
} Client;

struct pollfd fds[MAX_CLIENT + 1];
Client clients[MAX_CLIENT + 1];

/*----------------------------------------------------------*/
void trim(char *s)
{
    s[strcspn(s, "\r\n")] = 0;
}

/*----------------------------------------------------------*/
int checkUser(char *user, char *pass)
{
    FILE *f = fopen("users.txt", "r");
    if (f == NULL) return 0;

    char u[50], p[50];

    while (fscanf(f, "%s %s", u, p) == 2)
    {
        if (strcmp(user, u) == 0 &&
            strcmp(pass, p) == 0)
        {
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}

/*----------------------------------------------------------*/
void removeClient(int i)
{
    close(fds[i].fd);
    fds[i].fd = -1;

    clients[i].fd = -1;
    clients[i].state = 0;
}

/*----------------------------------------------------------*/
void runCommand(int fd, char *cmd)
{
    char syscmd[1200];
    char line[BUF_SIZE];

    snprintf(syscmd, sizeof(syscmd),
             "%s > out.txt", cmd);

    system(syscmd);

    FILE *f = fopen("out.txt", "r");

    if (f == NULL)
    {
        send(fd, "Command error!\n> ", 17, 0);
        return;
    }

    while (fgets(line, sizeof(line), f))
    {
        send(fd, line, strlen(line), 0);
    }

    fclose(f);

    send(fd, "\n> ", 3, 0);
}

/*----------------------------------------------------------*/
int main()
{
    int server_fd;
    struct sockaddr_in addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);

    printf("Telnet server running at port %d...\n", PORT);

    /* poll init */
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    for (int i = 1; i <= MAX_CLIENT; i++)
        fds[i].fd = -1;

    while (1)
    {
        poll(fds, MAX_CLIENT + 1, -1);

        /*------------------ New connection ------------------*/
        if (fds[0].revents & POLLIN)
        {
            int newfd = accept(server_fd, NULL, NULL);

            for (int i = 1; i <= MAX_CLIENT; i++)
            {
                if (fds[i].fd < 0)
                {
                    fds[i].fd = newfd;
                    fds[i].events = POLLIN;

                    clients[i].fd = newfd;
                    clients[i].state = 0;

                    send(newfd, "Username: ", 10, 0);
                    break;
                }
            }
        }

        /*------------------ Client message ------------------*/
        for (int i = 1; i <= MAX_CLIENT; i++)
        {
            if (fds[i].fd < 0) continue;

            if (fds[i].revents & POLLIN)
            {
                char buf[BUF_SIZE];

                int n = recv(fds[i].fd, buf,
                             sizeof(buf) - 1, 0);

                if (n <= 0)
                {
                    removeClient(i);
                    continue;
                }

                buf[n] = 0;
                trim(buf);

                /*----- Nhập username -----*/
                if (clients[i].state == 0)
                {
                    strcpy(clients[i].user, buf);
                    clients[i].state = 1;

                    send(fds[i].fd,
                         "Password: ",
                         10, 0);
                }

                /*----- Nhập password -----*/
                else if (clients[i].state == 1)
                {
                    strcpy(clients[i].pass, buf);

                    if (checkUser(clients[i].user,
                                  clients[i].pass))
                    {
                        clients[i].state = 2;

                        send(fds[i].fd,
                             "Login success!\n> ",
                             17, 0);
                    }
                    else
                    {
                        clients[i].state = 0;

                        send(fds[i].fd,
                             "Login failed!\nUsername: ",
                             25, 0);
                    }
                }

                /*----- Đã login -----*/
                else
                {
                    runCommand(fds[i].fd, buf);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}