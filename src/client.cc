#include "absl/log/check.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <unistd.h>

int main() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  CHECK(fd != -1) << "socket failed";
}