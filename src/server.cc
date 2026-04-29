#include "absl/log/check.h"
#include "absl/log/log.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <signal.h>
#include <unistd.h>

namespace {

constexpr size_t k_max_msg = 4096;
volatile sig_atomic_t stop = 0;

void handle_sigint(int) { stop = 1; }

// Helper functions to read/write the full buffer
int32_t read_full(int fd, char *buf, size_t n) {
  while (n > 0) {
    ssize_t rv = read(fd, buf, n);
    if (rv <= 0) {
      return -1; // error, or unexpected EOF
    }

    if ((size_t)rv > n) {
      LOG(ERROR) << "read returned more bytes than requested: " << rv << " > "
                 << n;
    }

    n -= (size_t)rv;
    buf += rv;
  }
  return 0;
}

int32_t write_all(int fd, const char *buf, size_t n) {
  while (n > 0) {
    ssize_t rv = write(fd, buf, n);
    if (rv <= 0) {
      return -1; // error
    }

    if ((size_t)rv > n) {
      LOG(ERROR) << "write returned more bytes than requested: " << rv << " > "
                 << n;
    }

    n -= (size_t)rv;
    buf += rv;
  }
  return 0;
}

int32_t one_request(int conn_fd) {
  // 4 bytes header
  char rbuf[4 + k_max_msg];
  errno = 0;
  int32_t err = read_full(conn_fd, rbuf, 4);
  if (err < 0) {
    LOG(ERROR) << "read header failed: "
               << (errno == 0 ? "EOF" : "read() error");
    return err;
  }

  uint32_t len = 0;
  memcpy(&len, rbuf, 4);

  if (len > k_max_msg) {
    LOG(ERROR) << "message too long";
    return -1;
  }
  // request body
  err = read_full(conn_fd, &rbuf[4], len);
  if (err) {
    LOG(ERROR) << "read() error";
    return err;
  }

  // do something
  LOG(INFO) << "client says: " << std::string(&rbuf[4], len);
  // reply using the same protocol
  const char reply[] = "world";
  char wbuf[4 + sizeof(reply)];
  len = (uint32_t)strlen(reply);
  memcpy(wbuf, &len, 4);
  memcpy(&wbuf[4], reply, len);
  return write_all(conn_fd, wbuf, 4 + len);
}

} // namespace

int main() {
  // Handle SIGINT to allow graceful shutdown
  signal(SIGINT, handle_sigint);

  // Create a TCP socket
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  CHECK(server_fd != -1) << "socket failed";

  sockaddr_in address = {
      .sin_family = AF_INET,
      .sin_addr = {.s_addr = INADDR_ANY}, // listen on all interfaces
      .sin_port = htons(8080)             // port 8080
  };

  CHECK(bind(server_fd, (sockaddr *)&address, sizeof(address)) == 0)
      << "bind failed";

  CHECK(listen(server_fd, 5) == 0) << "listen failed";

  LOG(INFO) << "Server listening on port 8080...";

  while (!stop) {
    socklen_t addr_len = sizeof(address);
    int client_fd = accept(server_fd, (sockaddr *)&address, &addr_len);
    if (client_fd == -1) {
      if (errno == EINTR && stop) {
        break;
      }
      continue; // Accept failed, try again
    }

    // only serves one client connection at once
    while (!stop) {
      int32_t err = one_request(client_fd);
      // If the client closed the connection, or an error occurred, stop serving
      // this client
      if (err < 0 || stop) {
        break;
      }
    }

    close(client_fd);
  }
  close(server_fd);
}
