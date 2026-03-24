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
    int sender = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8000);

    char buf[256];

    while (1) {
        printf("Enter string: ");
        fgets(buf, sizeof(buf), stdin);

        // bỏ '\n'
        buf[strcspn(buf, "\n")] = 0;

        if (strcmp(buf, "exit") == 0)
            break;

        int ret = sendto(sender, buf, strlen(buf), 0,
            (struct sockaddr *)&addr, sizeof(addr));

        printf("Sent %d bytes\n", ret);

        // Nhận echo
        int n = recvfrom(sender, buf, sizeof(buf) - 1, 0, NULL, NULL);
        buf[n] = 0;

        printf("Echo from server: %s\n", buf);
    }

    close(sender);
    return 0;
}