#include "daemon.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static zpaper_daemon_t global_daemon;

static void signal_handler(int signum) {
  (void)signum;
  if (global_daemon.wl_state && global_daemon.wl_state->display) {
    wl_display_cancel_read(global_daemon.wl_state->display);
  }
  daemon_stop(&global_daemon);
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  if (daemon_init(&global_daemon) != 0) {
    fprintf(stderr, "Failed to initialize daemon\n");
    return 1;
  }

  printf("Starting zpaper daemon...\n");
  int ret = daemon_start(&global_daemon);

  daemon_free(&global_daemon);
  return ret;
}
