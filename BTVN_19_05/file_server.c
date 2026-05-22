#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <dirent.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define STORAGE_DIR "./server_files" // Thư mục chứa các file trên server

// Hàm xử lý giao tiếp của từng Luồng (Thread) với Client
void *client_handler(void *arg) {
    int client_fd = *(int *)arg;
    free(arg); // Giải phóng bộ nhớ động cấp phát cho client_fd ở hàm main

    char buf[BUFFER_SIZE];
    char file_list[BUFFER_SIZE * 4] = {0};
    char filenames[100][256];
    int file_count = 0;

    printf("[Thread %ld] Đang phục vụ client_fd: %d\n", pthread_self(), client_fd);

    // 1. Đọc thư mục để lấy danh sách file (bỏ qua "." và "..")
    DIR *dir = opendir(STORAGE_DIR);
    if (dir == NULL) {
        char *err = "ERROR Cannot open storage directory\r\n";
        send(client_fd, err, strlen(err), 0);
        close(client_fd);
        pthread_exit(NULL);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            strncpy(filenames[file_count], entry->d_name, sizeof(filenames[file_count]) - 1);
            file_count++;
        }
    }
    closedir(dir);

    // 2. Nếu không có file nào trong thư mục thì báo lỗi và đóng kết nối ngay
    if (file_count == 0) {
        char *msg = "ERROR No files to download\r\n";
        send(client_fd, msg, strlen(msg), 0);
        close(client_fd);
        printf("[Thread %ld] Thư mục trống. Đã đóng kết nối.\n", pthread_self());
        pthread_exit(NULL);
    }

    // 3. Xây dựng chuỗi danh sách file theo đúng định dạng yêu cầu
    // Dòng đầu: OK N\r\n
    sprintf(file_list, "OK %d\r\n", file_count);
    
    // Tên các file phân cách bởi \r\n
    for (int i = 0; i < file_count; i++) {
        strcat(file_list, filenames[i]);
        strcat(file_list, "\r\n");
    }
    // Kết thúc danh sách bằng \r\n\r\n
    strcat(file_list, "\r\n");

    // Gửi danh sách file cho client
    send(client_fd, file_list, strlen(file_list), 0);

    // 4. Vòng lặp nhận yêu cầu tải file từ client
    while (1) {
        memset(buf, 0, sizeof(buf));
        int len = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (len <= 0) {
            printf("[Thread %ld] Client đã ngắt kết nối.\n", pthread_self());
            break;
        }

        buf[len] = '\0';
        
        // Cắt bỏ các ký tự xuống dòng (\r hoặc \n) do client gửi kèm
        buf[strcspn(buf, "\r\n")] = 0;

        // Tạo đường dẫn file hoàn chỉnh
        char filepath[2048];
        snprintf(filepath, sizeof(filepath), "%s/%s", STORAGE_DIR, buf);

        // Kiểm tra xem file có tồn tại và lấy kích thước file (stat)
        struct stat st;
        if (stat(filepath, &st) == 0 && S_ISREG(st.st_mode)) {
            // TRƯỜNG HỢP: FILE TỒN TẠI
            long file_size = st.st_size;
            printf("[Thread %ld] Client yêu cầu tải file: %s (%ld bytes)\n", pthread_self(), buf, file_size);

            // Gửi thông báo: OK N\r\n
            char header[64];
            sprintf(header, "OK %ld\r\n", file_size);
            send(client_fd, header, strlen(header), 0);

            // Đọc và gửi nội dung file
            FILE *f = fopen(filepath, "rb");
            if (f != NULL) {
                int read_bytes;
                while ((read_bytes = fread(buf, 1, sizeof(buf), f)) > 0) {
                    send(client_fd, buf, read_bytes, 0);
                }
                fclose(f);
            }

            // Gửi xong nội dung file thì đóng kết nối và thoát luồng theo yêu cầu
            printf("[Thread %ld] Đã gửi file xong. Đóng kết nối.\n", pthread_self());
            break; 
        } else {
            // TRƯỜNG HỢP: FILE KHÔNG TỒN TẠI
            char *err_msg = "ERROR: File không tồn tại hoặc sai tên. Vui lòng gửi lại tên file:\r\n";
            send(client_fd, err_msg, strlen(err_msg), 0);
            printf("[Thread %ld] File '%s' không tìm thấy. Đã yêu cầu client gửi lại.\n", pthread_self(), buf);
        }
    }

    close(client_fd);
    pthread_exit(NULL);
}

int main() {
    // Tự động tạo thư mục lưu trữ nếu chưa tồn tại
    mkdir(STORAGE_DIR, 0777);

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener < 0) {
        perror("socket() thất bại");
        return 1;
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind() thất bại");
        close(listener);
        return 1;
    }

    if (listen(listener, 10) < 0) {
        perror("listen() thất bại");
        close(listener);
        return 1;
    }

    printf("File Server (Đa luồng) đang chạy trên cổng %d...\n", PORT);
    printf("Hãy đặt các file cần chia sẻ vào thư mục: '%s'\n", STORAGE_DIR);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(listener, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept() thất bại");
            continue;
        }

        printf("Kết nối mới từ %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Cấp phát bộ nhớ động cho client_fd để tránh xung đột tài nguyên giữa các luồng (Race Condition)
        int *new_sock = malloc(sizeof(int));
        *new_sock = client_fd;

        pthread_t thread_id;
        // Tạo một luồng mới phục vụ client
        if (pthread_create(&thread_id, NULL, client_handler, (void *)new_sock) != 0) {
            perror("Không thể tạo luồng mới");
            close(client_fd);
            free(new_sock);
        } else {
            // Tách luồng để hệ thống tự động giải phóng tài nguyên khi luồng chạy xong
            pthread_detach(thread_id);
        }
    }

    close(listener);
    return 0;
}