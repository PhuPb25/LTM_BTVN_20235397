#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#define SERVER_HOST "lebavui.io.vn"
#define CONTROL_PORT 21
#define BUFFER_SIZE 8192

// --- CẤU HÌNH THÔNG TIN SINH VIÊN ---
#define MSSV "20235397"      
#define NGAY_SINH "25"       

// Hàm kết nối tới một Host và Port chỉ định
int connect_server(const char *host, int port) {
    struct hostent *he = gethostbyname(host);
    if (!he) {
        perror("[-] Không thể phân giải tên miền");
        return -1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *(struct in_addr *)he->h_addr;

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    return sock;
}

// Hàm gửi lệnh FTP và nhận phản hồi từ Control Channel
void send_ftp_cmd(int sock, const char *cmd, char *response_buf, int buf_len) {
    send(sock, cmd, strlen(cmd), 0);
    printf("Client: %s", cmd);

    memset(response_buf, 0, buf_len);
    int n = recv(sock, response_buf, buf_len - 1, 0);
    if (n > 0) {
        response_buf[n] = '\0';
        printf("Server: %s", response_buf);
    }
}

// Hàm gửi lệnh PASV và tính toán Port dữ liệu được cấp phát
int enter_passive_mode(int ctrl_sock) {
    char buf[BUFFER_SIZE];
    send_ftp_cmd(ctrl_sock, "PASV\r\n", buf, sizeof(buf));

    // Tìm vị trí chuỗi chứa IP và Port dạng (h1,h2,h3,h4,p1,p2)
    char *start = strchr(buf, '(');
    if (!start) return -1;

    int h1, h2, h3, h4, p1, p2;
    sscanf(start, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2);

    // Tính toán Port theo công thức của giao thức FTP
    int data_port = p1 * 256 + p2;
    return data_port;
}

int main() {
    int ctrl_sock, data_sock;
    char buf[BUFFER_SIZE];
    char cmd[512];
    
    char username[50];
    char password[50];

    // 1. TỰ ĐỘNG XỬ LÝ TÀI KHOẢN THEO ĐỀ BÀI
    sprintf(username, "user_%s", MSSV);
    // Lấy 4 số cuối MSSV
    char last_4_mssv[5];
    strncpy(last_4_mssv, MSSV + strlen(MSSV) - 4, 4);
    last_4_mssv[4] = '\0';
    sprintf(password, "%s%s", last_4_mssv, NGAY_SINH);

    printf("[*] Tài khoản login: %s / Mật khẩu: %s\n", username, password);

    // 2. KẾT NỐI KÊNH ĐIỀU KHIỂN (PORT 21)
    ctrl_sock = connect_server(SERVER_HOST, CONTROL_PORT);
    if (ctrl_sock < 0) {
        printf("[-] Kết nối đến Server FTP thất bại.\n");
        return -1;
    }
    // Nhận chuỗi chào mừng 220
    int n = recv(ctrl_sock, buf, sizeof(buf) - 1, 0);
    buf[n] = '\0';
    printf("Server: %s", buf);

    // 3. ĐĂNG NHẬP
    sprintf(cmd, "USER %s\r\n", username);
    send_ftp_cmd(ctrl_sock, cmd, buf, sizeof(buf));

    sprintf(cmd, "PASS %s\r\n", password);
    send_ftp_cmd(ctrl_sock, cmd, buf, sizeof(buf));

    if (strstr(buf, "230") == NULL) {
        printf("[-] Đăng nhập không thành công. Vui lòng kiểm tra lại MSSV/Ngày sinh.\n");
        close(ctrl_sock);
        return -1;
    }

    // 4. LẤY DANH SÁCH FILE ĐỂ TÌM FILE QUESTION
    int data_port = enter_passive_mode(ctrl_sock);
    data_sock = connect_server(SERVER_HOST, data_port);
    
    send_ftp_cmd(ctrl_sock, "NLST\r\n", buf, sizeof(buf)); // Nhận phản hồi 150

    // Đọc danh sách file từ Data Socket
    char file_list[BUFFER_SIZE] = {0};
    int total_bytes = 0;
    while ((n = recv(data_sock, file_list + total_bytes, sizeof(file_list) - total_bytes - 1, 0)) > 0) {
        total_bytes += n;
    }
    file_list[total_bytes] = '\0';
    close(data_sock); // Đóng data socket ngay sau khi nhận xong dữ liệu
    
    // Nhận phản hồi 226 trên kênh điều khiển
    n = recv(ctrl_sock, buf, sizeof(buf) - 1, 0); buf[n] = '\0'; printf("Server: %s", buf);

    // Trích xuất tên file question_xxxxxx.txt
    char question_file[100] = {0};
    char *q_ptr = strstr(file_list, "question_");
    if (!q_ptr) {
        printf("[-] Không tìm thấy file question_ trên server.\n");
        close(ctrl_sock);
        return -1;
    }
    char *q_end = strstr(q_ptr, ".txt");
    strncpy(question_file, q_ptr, q_end - q_ptr + 4);
    printf("[+] Xác định được file câu hỏi: %s\n", question_file);

    // 5. TẢI FILE QUESTION VỀ
    data_port = enter_passive_mode(ctrl_sock);
    data_sock = connect_server(SERVER_HOST, data_port);

    sprintf(cmd, "RETR %s\r\n", question_file);
    send_ftp_cmd(ctrl_sock, cmd, buf, sizeof(buf)); // Nhận phản hồi 150

    char question_content[150] = {0};
    total_bytes = 0;
    while ((n = recv(data_sock, question_content + total_bytes, sizeof(question_content) - total_bytes - 1, 0)) > 0) {
        total_bytes += n;
    }
    question_content[total_bytes] = '\0';
    close(data_sock);

    n = recv(ctrl_sock, buf, sizeof(buf) - 1, 0); buf[n] = '\0'; printf("Server: %s", buf);
    printf("[+] Nội dung file gốc nhận về: %s\n", question_content);

    // 6. ĐẢO NGƯỢC CHUỖI NỘI DUNG & TẠO TÊN FILE ĐÁP ÁN
    int len = strlen(question_content);
    // Loại bỏ ký tự xuống dòng nếu có ở cuối chuỗi từ server gửi về
    while(len > 0 && (question_content[len-1] == '\n' || question_content[len-1] == '\r')) {
        question_content[len-1] = '\0';
        len--;
    }

    char answer_content[150] = {0};
    for (int i = 0; i < len; i++) {
        answer_content[i] = question_content[len - 1 - i];
    }
    answer_content[len] = '\0';

    char answer_file[100];
    sprintf(answer_file, "answer_%s", question_file + 9); // Bỏ qua chữ "question_" (9 ký tự)
    printf("[*] File đáp án sẽ tạo: %s\n", answer_file);
    printf("[*] Nội dung đáp án đảo ngược: %s\n", answer_content);

    // 7. UPLOAD FILE ANSWER LÊN SERVER (STOR)
    data_port = enter_passive_mode(ctrl_sock);
    data_sock = connect_server(SERVER_HOST, data_port);

    sprintf(cmd, "STOR %s\r\n", answer_file);
    send_ftp_cmd(ctrl_sock, cmd, buf, sizeof(buf)); // Nhận phản hồi 150

    // Đẩy dữ liệu chuỗi đảo ngược lên Data Socket
    send(data_sock, answer_content, strlen(answer_content), 0);
    close(data_sock); // Đóng data socket để báo hiệu kết thúc file (EOF)

    n = recv(ctrl_sock, buf, sizeof(buf) - 1, 0); buf[n] = '\0'; printf("Server: %s", buf);

    // 8. ĐĂNG XUẤT
    send_ftp_cmd(ctrl_sock, "QUIT\r\n", buf, sizeof(buf));
    close(ctrl_sock);
    
    return 0;
}