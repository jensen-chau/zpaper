#include "ipc.h"
#include "wallpaper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *get_socket_path(void) {
  static char path[512];
  const char *xdg_cache = getenv("XDG_CACHE_HOME");
  if (xdg_cache) {
    snprintf(path, sizeof(path), "%s/%s", xdg_cache, IPC_SOCKET_NAME);
  } else {
    const char *home = getenv("HOME");
    snprintf(path, sizeof(path), "%s/.cache/%s", home ? home : "/tmp",
             IPC_SOCKET_NAME);
  }
  return path;
}

static int send_command(const char *cmd, const char *output, const char *path,
                        wallpaper_type_t type) {
  int fd;
  char *socket_path = get_socket_path();

  if (ipc_connect(socket_path, &fd) < 0) {
    fprintf(stderr, "Error: Cannot connect to zpaper daemon. Is it running?\n");
    return 1;
  }

  char buf[IPC_MAX_MSG_SIZE];
  if (path) {
    if (output) {
      snprintf(buf, sizeof(buf), "%s %s %s", cmd, path, output);
    } else {
      snprintf(buf, sizeof(buf), "%s %s", cmd, path);
    }
    if (type != WALLPAPER_TYPE_UNKNOWN) {
      size_t len = strlen(buf);
      snprintf(buf + len, sizeof(buf) - len, " --type=%s",
               wallpaper_type_to_string(type));
    }
  } else if (output) {
    snprintf(buf, sizeof(buf), "%s %s", cmd, output);
  } else {
    snprintf(buf, sizeof(buf), "%s", cmd);
  }

  size_t len = strlen(buf);
  buf[len] = '\n';
  if (ipc_write_msg(fd, buf, len + 1) < 0) {
    close(fd);
    fprintf(stderr, "Error: Failed to send command\n");
    return 1;
  }

  char resp[256];
  int n = read(fd, resp, sizeof(resp) - 1);
  if (n > 0) {
    resp[n] = '\0';
    printf("%s", resp);
  }

  close(fd);
  return 0;
}

static int start_daemon(void) {
  pid_t pid = fork();
  if (pid < 0) {
    fprintf(stderr, "Error: Failed to fork\n");
    return 1;
  }

  if (pid == 0) {
    execlp("zpaperd", "zpaperd", NULL);
    _exit(1);
  }

  sleep(1);
  printf("Daemon started\n");
  return 0;
}

static int stop_daemon(void) {
  return send_command("QUIT", NULL, NULL, WALLPAPER_TYPE_UNKNOWN);
}

static int show_status(void) {
  int fd;
  char *socket_path = get_socket_path();

  if (ipc_connect(socket_path, &fd) < 0) {
    printf("Daemon not running\n");
    return 1;
  }

  printf("Daemon running\n");
  close(fd);
  return 0;
}

static void usage(const char *prog) {
  printf("Usage: %s <command> [options]\n\n", prog);
  printf("Commands:\n");
  printf("  start                        Start the daemon\n");
  printf("  stop                         Stop the daemon\n");
  printf("  status                       Check daemon status\n");
  printf("  set [options] <path> [output]  Set wallpaper\n");
  printf("    --type=<type>  Wallpaper type: static, video, web, vulkan\n");
  printf("  get [output]                 Get current wallpaper\n");
  printf("  list                         List outputs and wallpapers\n");
  printf("  reload                       Reload configuration\n");
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    usage(argv[0]);
    return 1;
  }

  const char *cmd = argv[1];

  if (strcmp(cmd, "start") == 0) {
    return start_daemon();
  } else if (strcmp(cmd, "stop") == 0) {
    return stop_daemon();
  } else if (strcmp(cmd, "status") == 0) {
    return show_status();
  } else if (strcmp(cmd, "set") == 0) {
    wallpaper_type_t type = WALLPAPER_TYPE_UNKNOWN;
    const char *path = NULL;
    const char *output = NULL;

    for (int i = 2; i < argc; i++) {
      if (strncmp(argv[i], "--type=", 7) == 0) {
        type = wallpaper_type_from_string(argv[i] + 7);
        if (type == WALLPAPER_TYPE_UNKNOWN) {
          fprintf(stderr, "Error: Unknown wallpaper type '%s'\n", argv[i] + 7);
          return 1;
        }
      } else if (argv[i][0] != '-') {
        path = argv[i];
        if (i + 1 < argc && argv[i + 1][0] != '-') {
          output = argv[i + 1];
          i++;
        }
      }
    }

    if (!path) {
      fprintf(stderr, "Error: Missing wallpaper path\n");
      return 1;
    }
    return send_command("SET", output, path, type);
  } else if (strcmp(cmd, "get") == 0) {
    const char *output = (argc > 2) ? argv[2] : NULL;
    return send_command("GET", output, NULL, WALLPAPER_TYPE_UNKNOWN);
  } else if (strcmp(cmd, "list") == 0) {
    return send_command("LIST", NULL, NULL, WALLPAPER_TYPE_UNKNOWN);
  } else if (strcmp(cmd, "reload") == 0) {
    return send_command("RELOAD", NULL, NULL, WALLPAPER_TYPE_UNKNOWN);
  } else {
    fprintf(stderr, "Unknown command: %s\n", cmd);
    usage(argv[0]);
    return 1;
  }
}
