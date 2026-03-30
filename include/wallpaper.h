#ifndef ZPAPER_WALLPAPER_H
#define ZPAPER_WALLPAPER_H

#include "wayland_state.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
  WALLPAPER_TYPE_IMAGE,
  WALLPAPER_TYPE_VIDEO,
  WALLPAPER_TYPE_UNKNOWN
} wallpaper_type_t;

typedef struct wallpaper_handler wallpaper_handler_t;

typedef int (*wallpaper_load_fn)(wallpaper_handler_t *handler,
                                 const char *path);
typedef int (*wallpaper_render_fn)(wallpaper_handler_t *handler,
                                   struct output_info *output);
typedef void (*wallpaper_destroy_fn)(wallpaper_handler_t *handler);

struct wallpaper_handler {
  wallpaper_type_t type;
  const char *path;
  void *data;
  int src_width;
  int src_height;
  int rendered_outputs;
  int total_outputs;
  struct wayland_state *state;

  wallpaper_load_fn load;
  wallpaper_render_fn render;
  wallpaper_destroy_fn destroy;
};

wallpaper_type_t wallpaper_detect_type(const char *path);
wallpaper_handler_t *wallpaper_create(const char *path,
                                      struct wayland_state *state);
int wallpaper_render(wallpaper_handler_t *handler, struct output_info *output);
void wallpaper_destroy(wallpaper_handler_t *handler);

#endif
