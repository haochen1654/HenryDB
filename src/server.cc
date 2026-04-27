#include "absl/log/check.h"
#include "absl/log/log.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <signal.h>
#include <unistd.h>

volatile sig_atomic_t stop = 0;

void handle_sigint(int) { stop = 1; }

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
      CHECK(false) << "accept failed";
    }
    // Read data from the client
    char buffer[1024] = {0};

    if (read(client_fd, buffer, sizeof(buffer) - 1) > 0) {
      LOG(INFO) << "Client says: " << buffer;
      const char *response = "Hello from server!";
      send(client_fd, response, strlen(response), 0);
    }
    close(client_fd);
  }
  close(server_fd);
}
