#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>
 
#define BUF_SIZE      1024
#define MAX_USER_LEN  64
#define USERS_FILE    "users.txt"
#define END_MARKER    "<END>"
 
// Trạng thái FSM của mỗi client
typedef enum {
    STATE_WAIT_USER = 0,   // Đang chờ nhận username
    STATE_WAIT_PASS,       // Đang chờ nhận password
    STATE_LOGGED_IN        // Đã đăng nhập, chờ lệnh
} ClientState;
 
typedef struct {
    ClientState state;
    char username[MAX_USER_LEN];
    int  active;           // 1 nếu slot này đang được dùng
} Client;
 
Client clients[FD_SETSIZE];
 
// Xoá ký tự xuống dòng (\r, \n) ở cuối chuỗi
void trim_newline(char *s) {
    s[strcspn(s, "\r\n")] = '\0';
}
 
// Gửi toàn bộ dữ liệu, xử lý trường hợp send() gửi thiếu
int send_all(int fd, const char *buf, int len) {
    int sent = 0;
    while (sent < len) {
        int n = send(fd, buf + sent, len - sent, 0);
        if (n <= 0) return -1;
        sent += n;
    }
    return 0;
}
 
// Gửi chuỗi kết thúc '\0'
int send_str(int fd, const char *msg) {
    return send_all(fd, msg, strlen(msg));
}
 
// Kiểm tra cặp (user, pass) trong file users.txt
// Trả về 1 nếu hợp lệ, 0 nếu sai
int check_login(const char *user, const char *pass) {
    FILE *f = fopen(USERS_FILE, "r");
    if (!f) return 0;
 
    char u[MAX_USER_LEN], p[MAX_USER_LEN];
    int found = 0;
    while (fscanf(f, "%63s %63s", u, p) == 2) {
        if (strcmp(user, u) == 0 && strcmp(pass, p) == 0) {
            found = 1;
            break;
        }
    }
    fclose(f);
    return found;
}
 
// Thực thi lệnh cmd bằng system(), ghi output ra file tạm riêng cho từng fd,
// đọc file và gửi kết quả về client. Kết thúc bằng END_MARKER.
// Bọc lệnh trong (...) để tránh xung đột redirect khi lệnh client có dấu >
void execute_command(int fd, const char *cmd) {
    char outfile[64];
    char shell_cmd[BUF_SIZE + 64];
 
    // Tạo tên file output riêng cho từng fd để tránh ghi đè nhau
    snprintf(outfile, sizeof(outfile), "out_%d.txt", fd);
 
    // Dùng system() để thực hiện lệnh và định hướng đầu ra vào file tạm
    // Bọc trong (...) để redirect của client không xung đột với redirect server
    snprintf(shell_cmd, sizeof(shell_cmd), "(%s) > %s 2>&1", cmd, outfile);
    system(shell_cmd);
 
    // Đọc file kết quả và gửi về client
    FILE *f = fopen(outfile, "r");
    if (f) {
        char buf[BUF_SIZE];
        int n;
        while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
            send_all(fd, buf, n);
        }
        fclose(f);
        remove(outfile);   // Dọn file tạm sau khi gửi xong
    } else {
        send_str(fd, "(Không có output hoặc lệnh không tồn tại)\n");
    }
 
    // Gửi marker kết thúc để client biết đã nhận đủ dữ liệu
    send_str(fd, END_MARKER);
}
 
// Xử lý client ngắt kết nối
void disconnect_client(int fd, fd_set *master) {
    printf("Client fd=%d ngắt kết nối.\n", fd);
    close(fd);
    FD_CLR(fd, master);
    memset(&clients[fd], 0, sizeof(clients[fd]));
}
 
// Xử lý dữ liệu đến từ một client theo trạng thái FSM
void handle_client(int fd, fd_set *master) {
    char buf[BUF_SIZE];
    int  ret = recv(fd, buf, sizeof(buf) - 1, 0);
 
    if (ret <= 0) {
        // ret == 0: client đóng kết nối; ret < 0: lỗi mạng
        disconnect_client(fd, master);
        return;
    }
 
    buf[ret] = '\0';
    trim_newline(buf);
 
    switch (clients[fd].state) {
 
        case STATE_WAIT_USER:
            if (strlen(buf) == 0) {
                send_str(fd, "Username: ");
                break;
            }
            strncpy(clients[fd].username, buf, MAX_USER_LEN - 1);
            clients[fd].username[MAX_USER_LEN - 1] = '\0';
            clients[fd].state = STATE_WAIT_PASS;
            send_str(fd, "Password: ");
            break;
 
        case STATE_WAIT_PASS:
            if (check_login(clients[fd].username, buf)) {
                clients[fd].state = STATE_LOGGED_IN;
                send_str(fd, "Login successful!\r\n> ");
            } else {
                send_str(fd, "Login failed! Invalid username or password.\r\nUsername: ");
                clients[fd].state = STATE_WAIT_USER;
            }
            break;
 
        case STATE_LOGGED_IN:
            if (strcmp(buf, "exit") == 0 || strcmp(buf, "quit") == 0) {
                send_str(fd, "Goodbye!\r\n");
                disconnect_client(fd, master);
            } else if (strlen(buf) == 0) {
                send_str(fd, "> ");
            } else {
                execute_command(fd, buf);
                send_str(fd, "\r\n> ");
            }
            break;
    }
}
 
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }
 
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = INADDR_ANY;
 
    // Tái sử dụng địa chỉ ngay sau khi restart server
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
 
    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 10);
 
    fd_set master, readfds;
    FD_ZERO(&master);
    FD_SET(listener, &master);
    int maxfd = listener;
 
    memset(clients, 0, sizeof(clients));
 
    printf("Server đang chạy trên port %s...\n", argv[1]);
 
    while (1) {
        readfds = master;   // select() sẽ sửa readfds, phải copy mỗi lần
 
        int ready = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (ready < 0) {
            if (errno == EINTR) continue;
            perror("select()");
            break;
        }
 
        for (int fd = 0; fd <= maxfd; fd++) {
            if (!FD_ISSET(fd, &readfds)) continue;
 
            if (fd == listener) {
                // Có kết nối mới
                struct sockaddr_in cli_addr;
                socklen_t cli_len = sizeof(cli_addr);
                int newfd = accept(listener, (struct sockaddr *)&cli_addr, &cli_len);
                if (newfd < 0) { perror("accept()"); continue; }
 
                FD_SET(newfd, &master);
                if (newfd > maxfd) maxfd = newfd;
 
                clients[newfd].state  = STATE_WAIT_USER;
                clients[newfd].active = 1;
 
                send_str(newfd, "Username: ");
            } else {
                // Có dữ liệu từ client đã kết nối
                handle_client(fd, &master);
            }
        }
    }
 
    close(listener);
    return 0;
}