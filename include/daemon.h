#ifndef ZPAPER_DAEMON_H
#define ZPAPER_DAEMON_H

#include "config.h"
#include "wallpaper.h"
#include "wayland_state.h"
#include <stdbool.h>

typedef struct {
  struct wayland_state *wl_state;
  wallpaper_handler_t **wallpapers;
  int wallpaper_count;
  zpaper_config_t config;
  bool running;
  int socket_fd;
  char socket_path[512];
} zpaper_daemon_t;

int daemon_init(zpaper_daemon_t *daemon);
int daemon_start(zpaper_daemon_t *daemon);
void daemon_stop(zpaper_daemon_t *daemon);
void daemon_free(zpaper_daemon_t *daemon);
int daemon_set_wallpaper(zpaper_daemon_t *daemon, const char *output_name,
                         const char *path);
const char *daemon_get_wallpaper(zpaper_daemon_t *daemon,
                                 const char *output_name);
int daemon_reload_config(zpaper_daemon_t *daemon);

#endif
