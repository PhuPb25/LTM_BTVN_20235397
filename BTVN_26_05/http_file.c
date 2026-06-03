#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

// Hàm xác định Content-Type (MIME Type) dựa vào đuôi file
const char* get_mime_type(const char *path) {
    if (strstr(path, ".txt") || strstr(path, ".c")) return "text/plain; charset=UTF-8";
    if (strstr(path, ".html")) return "text/html; charset=UTF-8";
    if (strstr(path, ".jpg") || strstr(path, ".jpeg")) return "image/jpeg";
    if (strstr(path, ".png")) return "image/png";
    if (strstr(path, ".mp3")) return "audio/mpeg";
    if (strstr(path, ".mp4")) return "video/mp4";
    return "application/octet-stream"; // Kiểu nhị phân mặc định
}

int main() {
    // 1. Tạo Socket và Lắng nghe ở port 8081 (Tránh trùng với Bài 1)
    int server = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8081);

    bind(server, (struct sockaddr *)&addr, sizeof(addr));
    listen(server, 10);
    printf("File Server dang chay tai: http://localhost:8081\n");

    // 2. Vòng lặp xử lý kết nối
    while (1) {
        int client = accept(server, NULL, NULL);
        char buf[4096] = {0};
        recv(client, buf, sizeof(buf) - 1, 0);

        // Bóc tách Method và URL (Ví dụ: GET /Folder1 HTTP/1.1)
        char method[10] = {0}, url[1024] = {0};
        sscanf(buf, "%s %s", method, url);

        // Bỏ qua request biểu tượng favicon của trình duyệt
        if (strcmp(url, "/favicon.ico") == 0) {
            close(client);
            continue;
        }

        // Mẹo biến URL thành đường dẫn tương đối trên Server bằng cách thêm dấu '.'
        // Ví dụ: URL "/" thành "./", URL "/Anh/meo.jpg" thành "./Anh/meo.jpg"
        char actual_path[2048];
        snprintf(actual_path, sizeof(actual_path), ".%s", url);

        struct stat st;
        // Kiểm tra xem file/folder có tồn tại thật không
        if (stat(actual_path, &st) < 0) {
            char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            send(client, not_found, strlen(not_found), 0);
            close(client);
            continue;
        }

        // THƯ MỤC: Duyệt và trả về danh sách liên kết HTML
        if (S_ISDIR(st.st_mode)) {
            DIR *d = opendir(actual_path);
            char html[8192] = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
                              "<html><body style='font-family:Arial; margin:30px;'>"
                              "<h2>Danh sach Tep tin & Thu muc:</h2><ul>";
            
            struct dirent *dir;
            while ((dir = readdir(d)) != NULL) {
                // Ẩn thư mục ẩn "." và thư mục cha ".." cho gọn giao diện
                if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;

                // Lấy đường dẫn đầy đủ của phần tử con để kiểm tra loại tệp
                char sub_path[4096];
                snprintf(sub_path, sizeof(sub_path), "%s/%s", actual_path, dir->d_name);
                struct stat sub_st;
                stat(sub_path, &sub_st);

                // Tạo URL chính xác cho liên kết (bảo đảm cấu trúc /path/to/item)
                char link[2048];
                if (strcmp(url, "/") == 0) snprintf(link, sizeof(link), "/%s", dir->d_name);
                else snprintf(link, sizeof(link), "%s/%s", url, dir->d_name);

                char item[4096];
                if (S_ISDIR(sub_st.st_mode)) {
                    // Yêu cầu: THƯ MỤC THÌ IN ĐẬM <b>
                    snprintf(item, sizeof(item), "<li><a href='%s'><b>%s/</b></a></li>", link, dir->d_name);
                } else {
                    // Yêu cầu: FILE THÌ IN NGHIÊNG <i>
                    snprintf(item, sizeof(item), "<li><a href='%s'><i>%s</i></a></li>", link, dir->d_name);
                }
                strcat(html, item);
            }
            closedir(d);
            strcat(html, "</ul></body></html>");
            send(client, html, strlen(html), 0);
        }
        // FILE TRỰC TIẾP: Đọc dữ liệu nhị phân và đổ thẳng về trình duyệt
        else if (S_ISREG(st.st_mode)) {
            FILE *f = fopen(actual_path, "rb");
            if (f) {
                // 1. Gửi Header định dạng trước
                char header[512];
                snprintf(header, sizeof(header), 
                         "HTTP/1.1 200 OK\r\n"
                         "Content-Type: %s\r\n"
                         "Content-Length: %ld\r\n\r\n", 
                         get_mime_type(actual_path), st.st_size);
                send(client, header, strlen(header), 0);

                // 2. Stream dữ liệu theo từng cụm 4KB (Tránh quá tải RAM cho Server)
                char file_buf[4096];
                int bytes;
                while ((bytes = fread(file_buf, 1, sizeof(file_buf), f)) > 0) {
                    send(client, file_buf, bytes, 0);
                }
                fclose(f);
            }
        }
        close(client); // Đóng kết nối sau khi phục vụ xong request
    }

    close(server);
    return 0;
}