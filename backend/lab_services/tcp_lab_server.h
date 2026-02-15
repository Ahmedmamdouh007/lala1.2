#pragma once

/// Runs the TCP lab server (blocking). Listens on 127.0.0.1:9001 only.
/// Protocol: client sends LEN(2 bytes, big-endian) + DATA; server responds "OK" or "ERR".
/// Call from a separate thread when ENABLE_LABS=ON and LAB_MODE=true.
void run_tcp_lab_server();
