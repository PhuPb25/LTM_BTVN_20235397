#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

int main()
{
    int client = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8080);

    connect(client, (struct sockaddr *)&addr, sizeof(addr));

    printf("Connected to server...\n");

    struct pollfd fds[2];

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = client;
    fds[1].events = POLLIN;

    char buf[256];

    while (1)
    {
        int ret = poll(fds, 2, -1);

        if (ret < 0) break;

        /* keyboard */
        if (fds[0].revents & POLLIN)
        {
            fgets(buf, sizeof(buf), stdin);
            send(client, buf, strlen(buf), 0);
        }

        /* server */
        if (fds[1].revents & POLLIN)
        {
            ret = recv(client, buf, sizeof(buf)-1, 0);

            if (ret <= 0) break;

            buf[ret] = 0;
            printf("%s", buf);
            fflush(stdout);
        }
    }

    close(client);
    return 0;
}