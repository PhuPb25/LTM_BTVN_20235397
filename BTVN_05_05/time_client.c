#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main() {

    int client = socket(AF_INET,
                        SOCK_STREAM,
                        IPPROTO_TCP);

    if (client < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = {0};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(9001);
    addr.sin_addr.s_addr =
        inet_addr("127.0.0.1");

    if (connect(client,
                (struct sockaddr*)&addr,
                sizeof(addr)) < 0) {

        perror("connect");
        close(client);
        return 1;
    }

    printf("Connected to time server\n");

    char buf[BUFFER_SIZE];

    while (1) {

        printf("Enter command: ");

        fgets(buf,
              sizeof(buf),
              stdin);

        send(client,
             buf,
             strlen(buf),
             0);

        int len = recv(client,
                       buf,
                       sizeof(buf)-1,
                       0);

        if (len <= 0)
            break;

        buf[len] = 0;

        printf("Server: %s", buf);
    }

    close(client);

    return 0;
}