#include "ipc.h"
#include "utils.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static const char *cmd_strings[] = {"SET",    "GET",  "LIST", "RELOAD",
                                    "STATUS", "QUIT", "START"};

int ipc_connect(const char *socket_path, int *fd) {
  int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    return -1;
  }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

  if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(sock_fd);
    return -1;
  }

  *fd = sock_fd;
  return 0;
}

int ipc_accept(int server_fd, int *client_fd) {
  int fd = accept(server_fd, NULL, NULL);
  if (fd < 0) {
    return -1;
  }
  *client_fd = fd;
  return 0;
}

int ipc_read_msg(int fd, char *buf, size_t len) {
  size_t total_read = 0;
  while (total_read < len - 1) {
    ssize_t n = read(fd, buf + total_read, 1);
    if (n <= 0) {
      if (n < 0 && errno == EINTR) {
        continue;
      }
      return -1;
    }
    if (buf[total_read] == '\n') {
      buf[total_read] = '\0';
      return 0;
    }
    total_read++;
  }
  buf[total_read] = '\0';
  return 0;
}

int ipc_write_msg(int fd, const char *buf, size_t len) {
  size_t total_written = 0;
  while (total_written < len) {
    ssize_t n = write(fd, buf + total_written, len - total_written);
    if (n <= 0) {
      if (n < 0 && errno == EINTR) {
        continue;
      }
      return -1;
    }
    total_written += n;
  }
  return 0;
}

ipc_cmd_t ipc_parse_cmd(const char *cmd_str) {
  if (!cmd_str) {
    return IPC_CMD_UNKNOWN;
  }
  for (int i = 0; i < (int)(sizeof(cmd_strings) / sizeof(cmd_strings[0]));
       i++) {
    if (strcasecmp(cmd_str, cmd_strings[i]) == 0) {
      return (ipc_cmd_t)i;
    }
  }
  return IPC_CMD_UNKNOWN;
}

const char *ipc_cmd_to_string(ipc_cmd_t cmd) {
  if (cmd >= 0 && cmd < (int)(sizeof(cmd_strings) / sizeof(cmd_strings[0]))) {
    return cmd_strings[cmd];
  }
  return "UNKNOWN";
}

void ipc_request_free(ipc_request_t *req) {
  if (!req) {
    return;
  }
  free(req->wallpaper_path);
  free(req->output_name);
  free(req->payload);
  free(req);
}

void ipc_response_free(ipc_response_t *resp) {
  if (!resp) {
    return;
  }
  free(resp->message);
  free(resp->data);
  free(resp);
}
