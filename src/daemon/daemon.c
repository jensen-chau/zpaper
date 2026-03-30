#include "daemon.h"
#include "ipc.h"
#include "utils.h"
#include "wl_shm_pool.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

static void daemon_signal_handler(int signum) { (void)signum; }

static int create_socket(const char *socket_path) {
  unlink(socket_path);

  int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    return -1;
  }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

  if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(sock_fd);
    return -1;
  }

  chmod(socket_path, 0600);

  if (listen(sock_fd, 5) < 0) {
    close(sock_fd);
    return -1;
  }

  return sock_fd;
}

static int daemon_render_all(zpaper_daemon_t *daemon) {
  for (int i = 0; i < daemon->wl_state->output_count; i++) {
    struct output_info *output = daemon->wl_state->output_infos[i];
    const char *wallpaper_path =
        config_get_wallpaper(&daemon->config, output->name);
    wallpaper_type_t wallpaper_type =
        config_get_wallpaper_type(&daemon->config, output->name);

    if (!wallpaper_path) {
      continue;
    }

    if (output->shm_pool) {
      shm_pool_destroy(output->shm_pool);
      output->shm_pool = NULL;
    }

    wallpaper_handler_t *handler =
        wallpaper_create(wallpaper_path, daemon->wl_state, wallpaper_type);
    if (!handler) {
      ERR("failed to create wallpaper handler for %s", wallpaper_path);
      continue;
    }

    if (wallpaper_render(handler, output) != 0) {
      ERR("failed to render wallpaper for %s", output->name);
      wallpaper_destroy(handler);
      continue;
    }

    wallpaper_destroy(handler);
  }
  return 0;
}

static int handle_ipc_command(zpaper_daemon_t *daemon, ipc_cmd_t cmd,
                              const char *output_name,
                              const char *wallpaper_path,
                              wallpaper_type_t wallpaper_type,
                              char *response_buf, size_t resp_size) {
  switch (cmd) {
  case IPC_CMD_SET:
    if (daemon_set_wallpaper(daemon, output_name, wallpaper_path,
                             wallpaper_type) == 0) {
      config_save(config_get_default_path(), &daemon->config);
      daemon_render_all(daemon);
      wl_display_flush(daemon->wl_state->display);
      snprintf(response_buf, resp_size, "OK\n");
      return 0;
    }
    snprintf(response_buf, resp_size, "ERROR: Failed to set wallpaper\n");
    return -1;

  case IPC_CMD_GET: {
    const char *wallpaper = daemon_get_wallpaper(daemon, output_name);
    if (wallpaper) {
      snprintf(response_buf, resp_size, "%s\n", wallpaper);
    } else {
      snprintf(response_buf, resp_size, "No wallpaper set\n");
    }
    return 0;
  }

  case IPC_CMD_LIST: {
    char *p = response_buf;
    size_t remaining = resp_size;
    int printed = 0;

    for (int i = 0; i < daemon->wl_state->output_count; i++) {
      struct output_info *output = daemon->wl_state->output_infos[i];
      const char *wallpaper =
          config_get_wallpaper(&daemon->config, output->name);
      wallpaper_type_t type =
          config_get_wallpaper_type(&daemon->config, output->name);
      int len;
      if (wallpaper) {
        len = snprintf(p, remaining, "%s [%s]: %s\n", output->name,
                       wallpaper_type_to_string(type), wallpaper);
      } else {
        len = snprintf(p, remaining, "%s: (none)\n", output->name);
      }
      if (len > 0 && (size_t)len < remaining) {
        p += len;
        remaining -= len;
        printed++;
      } else {
        break;
      }
    }
    if (printed == 0) {
      snprintf(response_buf, resp_size, "No outputs\n");
    }
    return 0;
  }

  case IPC_CMD_RELOAD:
    if (daemon_reload_config(daemon) == 0) {
      snprintf(response_buf, resp_size, "OK\n");
      return 0;
    }
    snprintf(response_buf, resp_size, "ERROR: Failed to reload config\n");
    return -1;

  case IPC_CMD_QUIT:
    daemon->running = false;
    snprintf(response_buf, resp_size, "OK\n");
    return 0;

  default:
    snprintf(response_buf, resp_size, "ERROR: Unknown command\n");
    return -1;
  }
}

int daemon_init(zpaper_daemon_t *daemon) {
  memset(daemon, 0, sizeof(*daemon));

  const char *xdg_cache = getenv("XDG_CACHE_HOME");
  if (xdg_cache) {
    snprintf(daemon->socket_path, sizeof(daemon->socket_path), "%s/%s",
             xdg_cache, IPC_SOCKET_NAME);
  } else {
    const char *home = getenv("HOME");
    snprintf(daemon->socket_path, sizeof(daemon->socket_path), "%s/.cache/%s",
             home ? home : "/tmp", IPC_SOCKET_NAME);
  }

  char *cache_dir = strdup(daemon->socket_path);
  char *last_slash = strrchr(cache_dir, '/');
  if (last_slash) {
    *last_slash = '\0';
    mkdir(cache_dir, 0755);
  }
  free(cache_dir);

  config_init(&daemon->config);
  const char *config_path = config_get_default_path();
  config_load(config_path, &daemon->config);

  return 0;
}

int daemon_start(zpaper_daemon_t *daemon) {
  daemon->socket_fd = create_socket(daemon->socket_path);
  if (daemon->socket_fd < 0) {
    ERR("failed to create socket at %s", daemon->socket_path);
    return -1;
  }

  signal(SIGINT, daemon_signal_handler);
  signal(SIGTERM, daemon_signal_handler);

  daemon->wl_state = wayland_state_new();
  if (!daemon->wl_state) {
    ERR("failed to initialize wayland state");
    close(daemon->socket_fd);
    return -1;
  }

  while (daemon->wl_state->output_count == 0) {
    wl_display_dispatch(daemon->wl_state->display);
  }

  while (1) {
    int all_configured = 1;
    for (int i = 0; i < daemon->wl_state->output_count; i++) {
      if (!daemon->wl_state->output_infos[i]->is_configured) {
        all_configured = 0;
        break;
      }
    }
    if (all_configured) {
      break;
    }
    wl_display_dispatch(daemon->wl_state->display);
  }

  if (daemon->config.default_wallpaper || daemon->config.output_count > 0) {
    daemon_render_all(daemon);
    wl_display_flush(daemon->wl_state->display);
  }

  daemon->running = true;
  while (daemon->running) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(daemon->socket_fd, &fds);
    FD_SET(wl_display_get_fd(daemon->wl_state->display), &fds);

    int max_fd =
        (daemon->socket_fd > wl_display_get_fd(daemon->wl_state->display))
            ? daemon->socket_fd
            : wl_display_get_fd(daemon->wl_state->display);

    struct timeval tv = {0, 10000};
    int ready = select(max_fd + 1, &fds, NULL, NULL, &tv);

    if (ready < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }

    if (FD_ISSET(wl_display_get_fd(daemon->wl_state->display), &fds)) {
      wl_display_dispatch(daemon->wl_state->display);
    }

    if (FD_ISSET(daemon->socket_fd, &fds)) {
      int client_fd;
      if (ipc_accept(daemon->socket_fd, &client_fd) >= 0) {
        char buf[IPC_MAX_MSG_SIZE];
        if (ipc_read_msg(client_fd, buf, sizeof(buf) - 1) >= 0) {
          buf[sizeof(buf) - 1] = '\0';

          char cmd_str[64] = {0};
          char path_arg[MAX_PATH_LEN] = {0};
          char output_name[128] = {0};
          char type_str[64] = {0};
          wallpaper_type_t wallpaper_type = WALLPAPER_TYPE_UNKNOWN;

          char *saveptr = NULL;
          char *token = strtok_r(buf, " ", &saveptr);
          if (token) {
            strncpy(cmd_str, token, sizeof(cmd_str) - 1);
            cmd_str[sizeof(cmd_str) - 1] = '\0';
          }

          token = strtok_r(NULL, " ", &saveptr);
          if (token) {
            if (strncmp(token, "--type=", 7) == 0) {
              strncpy(type_str, token + 7, sizeof(type_str) - 1);
              wallpaper_type = wallpaper_type_from_string(type_str);
            } else {
              strncpy(path_arg, token, sizeof(path_arg) - 1);
              path_arg[sizeof(path_arg) - 1] = '\0';
            }
          }

          token = strtok_r(NULL, " ", &saveptr);
          if (token) {
            if (strncmp(token, "--type=", 7) == 0) {
              strncpy(type_str, token + 7, sizeof(type_str) - 1);
              wallpaper_type = wallpaper_type_from_string(type_str);
            } else {
              strncpy(output_name, token, sizeof(output_name) - 1);
              output_name[sizeof(output_name) - 1] = '\0';
            }
          }

          token = strtok_r(NULL, " ", &saveptr);
          if (token && strncmp(token, "--type=", 7) == 0) {
            strncpy(type_str, token + 7, sizeof(type_str) - 1);
            wallpaper_type = wallpaper_type_from_string(type_str);
          }

          if (path_arg[0] == '~' && path_arg[1] == '/') {
            const char *home = getenv("HOME");
            if (home) {
              char expanded[512];
              snprintf(expanded, sizeof(expanded), "%s%s", home, path_arg + 1);
              strncpy(path_arg, expanded, sizeof(path_arg) - 1);
            }
          }

          ipc_cmd_t cmd = ipc_parse_cmd(cmd_str);
          char response[IPC_MAX_MSG_SIZE];
          handle_ipc_command(daemon, cmd, output_name[0] ? output_name : NULL,
                             path_arg[0] ? path_arg : NULL, wallpaper_type,
                             response, sizeof(response));

          ipc_write_msg(client_fd, response, strlen(response));
        }
        close(client_fd);
      }
    }
  }

  return 0;
}

void daemon_stop(zpaper_daemon_t *daemon) { daemon->running = false; }

void daemon_free(zpaper_daemon_t *daemon) {
  if (daemon->wallpapers) {
    for (int i = 0; i < daemon->wallpaper_count; i++) {
      wallpaper_destroy(daemon->wallpapers[i]);
    }
    free(daemon->wallpapers);
  }

  config_free(&daemon->config);

  if (daemon->socket_fd >= 0) {
    close(daemon->socket_fd);
    unlink(daemon->socket_path);
  }
}

int daemon_set_wallpaper(zpaper_daemon_t *daemon, const char *output_name,
                         const char *path, wallpaper_type_t type) {
  if (output_name) {
    return config_set_wallpaper(&daemon->config, output_name, path, type);
  }

  for (int i = 0; i < daemon->wl_state->output_count; i++) {
    const char *output_name_i = daemon->wl_state->output_infos[i]->name;
    config_set_wallpaper(&daemon->config, output_name_i, path, type);
  }
  return 0;
}

const char *daemon_get_wallpaper(zpaper_daemon_t *daemon,
                                 const char *output_name) {
  return config_get_wallpaper(&daemon->config, output_name);
}

int daemon_reload_config(zpaper_daemon_t *daemon) {
  config_free(&daemon->config);
  config_init(&daemon->config);
  return config_load(config_get_default_path(), &daemon->config);
}
