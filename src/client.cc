#include "absl/log/check.h"
#include "absl/log/log.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
namespace {

constexpr size_t k_max_msg = 4096;

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

int32_t query(int fd, const char *text) {
  uint32_t len = (uint32_t)strlen(text);
  if (len > k_max_msg) {
    return -1;
  }
  // send request
  char wbuf[4 + k_max_msg];
  memcpy(wbuf, &len, 4); // assume little endian
  memcpy(&wbuf[4], text, len);
  if (int32_t err = write_all(fd, wbuf, 4 + len)) {
    return err;
  }

  // 4 bytes header
  char rbuf[4 + k_max_msg];
  errno = 0;
  int32_t err = read_full(fd, rbuf, 4);
  if (err) {
    LOG(ERROR) << (errno == 0 ? "EOF" : "read() error");
    return err;
  }

  memcpy(&len, rbuf, 4); // assume little endian

  if (len > k_max_msg) {
    LOG(ERROR) << "reply too long";
    return -1;
  }
  // reply body
  err = read_full(fd, &rbuf[4], len);
  if (err) {
    LOG(ERROR) << "read() error";
    return err;
  }
  // do something
  LOG(INFO) << "Server says: " << std::string(&rbuf[4], len);
  return 0;
}

} // namespace

int main() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  CHECK(fd != -1) << "socket failed";

  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_port = ntohs(8080),
      .sin_addr = {.s_addr = ntohl(INADDR_LOOPBACK)}}; // 127.0.0.1

  CHECK(connect(fd, (const struct sockaddr *)&addr, sizeof(addr)) == 0)
      << "connect failed";

  // multiple requests
  int32_t err = query(fd, "hello1");
  if (err) {
    close(fd);
    return err;
  }

  err = query(fd, "hello2");
  if (err) {
    close(fd);
    return err;
  }

  err = query(fd, "hello3");
  if (err) {
    close(fd);
    return err;
  }

  close(fd);
  return 0;
}