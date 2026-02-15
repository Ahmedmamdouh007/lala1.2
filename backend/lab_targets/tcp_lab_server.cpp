/**
 * TCP lab server - Educational / CTF target only.
 * Listens on port 9999. Reads one "message" per connection into a small buffer.
 * Intentionally vulnerable: recv() can write past the buffer (for boofuzz + ASan demo).
 *
 * Build with ASan:
 *   clang++ -fsanitize=address -g -fno-omit-frame-pointer -o tcp_lab_server tcp_lab_server.cpp
 *
 * Run: ./tcp_lab_server
 * Then fuzz with: python backend/lab_targets/boofuzz_tcp_lab.py
 */

#include <cstring>
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {

const int PORT = 9999;
const size_t PARSER_BUF_SIZE = 16;  // Small buffer so overflow is easy to trigger

void handle_client(int fd) {
    // Vulnerable parser: fixed-size buffer but recv with large size (no bound check)
    char buf[PARSER_BUF_SIZE];
    ssize_t n = recv(fd, buf, 4096, 0);  // BUG: 4096 > sizeof(buf) -> stack-buffer-overflow
    if (n > 0) {
        buf[n] = '\0';  // potential overflow if n >= PARSER_BUF_SIZE
    }
    (void)buf;
    const char* reply = "OK\n";
    send(fd, reply, 3, 0);
}

}  // namespace

int main() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }
#endif

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "socket failed\n";
        return 1;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "bind failed\n";
        return 1;
    }
    if (listen(sock, 5) < 0) {
        std::cerr << "listen failed\n";
        return 1;
    }

    std::cout << "TCP lab server listening on 0.0.0.0:" << PORT << " (ASan build; fuzz with boofuzz_tcp_lab.py)\n";

    for (;;) {
        struct sockaddr_in client_addr = {};
        socklen_t len = sizeof(client_addr);
        int client = accept(sock, (struct sockaddr*)&client_addr, &len);
        if (client < 0) continue;
        handle_client(client);
#ifdef _WIN32
        closesocket(client);
#else
        close(client);
#endif
    }

#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
    return 0;
}
