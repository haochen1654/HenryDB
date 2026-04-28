#include "absl/log/check.h"
#include "absl/log/log.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <unistd.h>

int main() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  CHECK(fd != -1) << "socket failed";

  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_port = ntohs(8080),
      .sin_addr = {.s_addr = ntohl(INADDR_LOOPBACK)}}; // 127.0.0.1

  CHECK(connect(fd, (const struct sockaddr *)&addr, sizeof(addr)) == 0)
      << "connect failed";

  // Send a message to the server
  char msg[] = "hello";
  write(fd, msg, strlen(msg));

  // Read the response from the server
  char rbuf[64] = {};
  CHECK(read(fd, rbuf, sizeof(rbuf) - 1) != -1) << "read failed";

  LOG(INFO) << "Server says: " << rbuf;

  close(fd);
}