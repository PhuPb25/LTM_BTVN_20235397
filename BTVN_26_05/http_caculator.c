#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main() {
    // 1. Tạo Socket và Lắng nghe ở port 8080
    int server = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); 

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    bind(server, (struct sockaddr *)&addr, sizeof(addr));
    listen(server, 5);
    printf("Server dang chay tai: http://localhost:8080\n");

    // 2. Vòng lặp nhận Request
    while (1) {
        int client = accept(server, NULL, NULL);
        char buf[4096] = {0};
        recv(client, buf, sizeof(buf) - 1, 0);

        // Tìm kiếm tham số a, b, op trong Request
        char *pA = strstr(buf, "a=");
        char *pB = strstr(buf, "b=");
        char *pOp = strstr(buf, "op=");

        char response[2048] = {0};

        // NẾU CÓ THAM SỐ: Tính toán và trả về kết quả
        if (pA && pB && pOp) {
            double a = atof(pA + 2); 
            double b = atof(pB + 2); 
            double res = 0;
            char op_char = '+';

            if (strncmp(pOp + 3, "add", 3) == 0) { res = a + b; op_char = '+'; }
            else if (strncmp(pOp + 3, "sub", 3) == 0) { res = a - b; op_char = '-'; }
            else if (strncmp(pOp + 3, "mul", 3) == 0) { res = a * b; op_char = '*'; }
            else if (strncmp(pOp + 3, "div", 3) == 0) { res = (b != 0) ? a / b : 0; op_char = '/'; }

            // Trả về trang hiển thị kết quả (Vẫn giữ link để quay lại)
            snprintf(response, sizeof(response),
                "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
                "<div style='font-family:Arial; text-align:center; margin-top:50px;'>"
                "  <h2>Ket qua phep tinh:</h2>"
                "  <p style='font-size:24px; color:blue;'><b>%.2f %c %.2f = %.2f</b></p>"
                "  <br><a href='/'>Quay lai trang tinh toán</a>"
                "</div>", a, op_char, b, res);
        } 
        // NẾU KHÔNG CÓ THAM SỐ (Trang chủ): Hiển thị 1 FORM DUY NHẤT
        else {
            snprintf(response, sizeof(response),
                "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
                "<html><body style='font-family:Arial; max-width:400px; margin:50px auto;'>"
                "  <h2>May tinh HTTP</h2>"
                "  "
                "  <form method='GET' action='/' onsubmit=\"this.method=document.getElementById('method_select').value;\">"
                "    <p>"
                "      <b>Chon phuong thuc:</b> "
                "      <select id='method_select'><option value='GET'>Gửi bằng GET</option><option value='POST'>Gửi bằng POST</option></select>"
                "    </p>"
                "    <p>Nhập số a: <input name='a' type='number' required></p>"
                "    <p>Toán tử: "
                "      <select name='op'>"
                "        <option value='add'>+</option><option value='sub'>-</option>"
                "        <option value='mul'>*</option><option value='div'>/</option>"
                "      </select>"
                "    </p>"
                "    <p>Nhập số b: <input name='b' type='number' required></p>"
                "    <input type='submit' value='Thuc hien tinh' style='padding:5px 15px;'>"
                "  </form>"
                "</body></html>");
        }

        // Gửi dữ liệu về và đóng kết nối
        send(client, response, strlen(response), 0);
        close(client);
    }

    close(server);
    return 0;
}