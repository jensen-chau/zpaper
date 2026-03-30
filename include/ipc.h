#ifndef ZPAPER_IPC_H
#define ZPAPER_IPC_H

#include "wallpaper.h"
#include <stddef.h>

#define IPC_SOCKET_NAME "zpaper.sock"
#define IPC_MAX_MSG_SIZE 4096

typedef enum {
  IPC_CMD_SET,
  IPC_CMD_GET,
  IPC_CMD_LIST,
  IPC_CMD_RELOAD,
  IPC_CMD_STATUS,
  IPC_CMD_QUIT,
  IPC_CMD_START,
  IPC_CMD_UNKNOWN
} ipc_cmd_t;

typedef struct {
  ipc_cmd_t cmd;
  char *wallpaper_path;
  char *output_name;
  wallpaper_type_t wallpaper_type;
  char *payload;
  size_t payload_size;
} ipc_request_t;

typedef struct {
  int success;
  char *message;
  char *data;
  size_t data_size;
} ipc_response_t;

int ipc_connect(const char *socket_path, int *fd);
int ipc_accept(int server_fd, int *client_fd);
int ipc_read_msg(int fd, char *buf, size_t len);
int ipc_write_msg(int fd, const char *buf, size_t len);
ipc_cmd_t ipc_parse_cmd(const char *cmd_str);
const char *ipc_cmd_to_string(ipc_cmd_t cmd);
void ipc_request_free(ipc_request_t *req);
void ipc_response_free(ipc_response_t *resp);

#endif
