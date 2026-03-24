#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

int main() {
    int receiver = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8000);

    bind(receiver, (struct sockaddr *)&addr, sizeof(addr));

    char buf[256];
    struct sockaddr_in s_addr;
    int s_addr_len = sizeof(s_addr);

    while (1) {
        int ret = recvfrom(receiver, buf, sizeof(buf) - 1, 0,
            (struct sockaddr *)&s_addr, &s_addr_len);

        if (ret <= 0)
            break;

        buf[ret] = 0;

        printf("Received %d bytes from %s:%d: %s\n", 
            ret, 
            inet_ntoa(s_addr.sin_addr),
            ntohs(s_addr.sin_port),
            buf);

        //Echo lại
        sendto(receiver, buf, ret, 0,
            (struct sockaddr *)&s_addr, s_addr_len);
    }

    close(receiver);
    return 0;
}