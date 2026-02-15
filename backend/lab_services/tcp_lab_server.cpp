/**
 * TCP lab service (NOT HTTP). Only started when ENABLE_LABS=ON and LAB_MODE=true.
 * Listens on 127.0.0.1:9001 only.
 * Protocol: client sends LEN(2 bytes, big-endian) + DATA; server responds "OK" or "ERR".
 */

#include "tcp_lab_server.h"
#include <cstring>
#include <iostream>
#include <vector>
#include <cstdint>

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

const char* BIND_HOST = "127.0.0.1";
const int BIND_PORT = 9001;
constexpr uint32_t MAX_DATA_LEN = 65535;

#ifdef _WIN32
using sock_t = SOCKET;
inline int close_sock(sock_t s) { return closesocket(s); }
#else
using sock_t = int;
inline int close_sock(sock_t s) { return close(s); }
#endif

void handle_client(sock_t fd) {
    unsigned char len_buf[2];
    int n = recv(fd, reinterpret_cast<char*>(len_buf), 2, 0);
    if (n != 2) {
        send(fd, "ERR", 3, 0);
        return;
    }
    uint32_t data_len = (static_cast<uint32_t>(len_buf[0]) << 8) | len_buf[1];
    if (data_len > MAX_DATA_LEN) {
        send(fd, "ERR", 3, 0);
        return;
    }
    std::vector<char> data(data_len);
    size_t total = 0;
    while (total < data_len) {
        n = recv(fd, data.data() + total, static_cast<int>(data_len - total), 0);
        if (n <= 0) {
            send(fd, "ERR", 3, 0);
            return;
        }
        total += static_cast<size_t>(n);
    }
    send(fd, "OK", 2, 0);
}

void server_loop(sock_t listen_fd) {
    for (;;) {
        struct sockaddr_in client_addr = {};
        socklen_t len = sizeof(client_addr);
        sock_t client = accept(listen_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &len);
        if (client == (sock_t)-1
#ifdef _WIN32
            || client == INVALID_SOCKET
#endif
        ) continue;
        handle_client(client);
        close_sock(client);
    }
}

} // namespace

void run_tcp_lab_server() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "TCP lab server: WSAStartup failed\n";
        return;
    }
#endif

    sock_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == (sock_t)-1
#ifdef _WIN32
        || sock == INVALID_SOCKET
#endif
    ) {
        std::cerr << "TCP lab server: socket failed\n";
        return;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(BIND_PORT));
    if (inet_pton(AF_INET, BIND_HOST, &addr.sin_addr) <= 0) {
        std::cerr << "TCP lab server: inet_pton failed\n";
        close_sock(sock);
        return;
    }

    if (bind(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "TCP lab server: bind 127.0.0.1:" << BIND_PORT << " failed\n";
        close_sock(sock);
        return;
    }
    if (listen(sock, 5) < 0) {
        std::cerr << "TCP lab server: listen failed\n";
        close_sock(sock);
        return;
    }

    std::cout << "TCP lab service listening on 127.0.0.1:" << BIND_PORT << " (LEN(2)+DATA -> OK/ERR)\n";
    server_loop(sock);
    close_sock(sock);

#ifdef _WIN32
    WSACleanup();
#endif
}
