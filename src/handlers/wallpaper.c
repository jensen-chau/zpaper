#include "wallpaper.h"
#include "image_handler.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

static const char *image_extensions[] = {".png", ".jpg", ".jpeg",
                                         ".bmp", ".gif", ".webp"};
static const int image_ext_count =
    sizeof(image_extensions) / sizeof(image_extensions[0]);

static const char *video_extensions[] = {".mp4", ".webm", ".mkv", ".avi",
                                         ".mov"};
static const int video_ext_count =
    sizeof(video_extensions) / sizeof(video_extensions[0]);

wallpaper_type_t wallpaper_detect_type(const char *path) {
  if (!path) {
    return WALLPAPER_TYPE_UNKNOWN;
  }

  const char *ext = strrchr(path, '.');
  if (!ext) {
    return WALLPAPER_TYPE_UNKNOWN;
  }

  for (int i = 0; i < image_ext_count; i++) {
    if (strcasecmp(ext, image_extensions[i]) == 0) {
      return WALLPAPER_TYPE_IMAGE;
    }
  }

  for (int i = 0; i < video_ext_count; i++) {
    if (strcasecmp(ext, video_extensions[i]) == 0) {
      return WALLPAPER_TYPE_VIDEO;
    }
  }

  return WALLPAPER_TYPE_UNKNOWN;
}

wallpaper_handler_t *wallpaper_create(const char *path,
                                      struct wayland_state *state) {
  wallpaper_type_t type = wallpaper_detect_type(path);

  wallpaper_handler_t *handler = calloc(1, sizeof(wallpaper_handler_t));
  if (!handler) {
    return NULL;
  }

  handler->type = type;
  handler->path = path;
  handler->state = state;

  switch (type) {
  case WALLPAPER_TYPE_IMAGE:
    if (image_handler_init(handler, path) != 0) {
      free(handler);
      return NULL;
    }
    break;

  case WALLPAPER_TYPE_VIDEO:
    ERR("video wallpaper not yet implemented");
    free(handler);
    return NULL;

  default:
    ERR("unsupported wallpaper type");
    free(handler);
    return NULL;
  }

  return handler;
}

int wallpaper_render(wallpaper_handler_t *handler, struct output_info *output) {
  if (!handler || !handler->render) {
    return -1;
  }
  return handler->render(handler, output);
}

void wallpaper_destroy(wallpaper_handler_t *handler) {
  if (!handler) {
    return;
  }
  if (handler->destroy) {
    handler->destroy(handler);
  }
  free(handler);
}
