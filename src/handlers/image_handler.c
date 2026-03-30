#define STB_IMAGE_IMPLEMENTATION
#include "image_handler.h"
#include "stb_image.h"
#include "utils.h"
#include "wl_shm_pool.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  uint8_t *data;
  int width;
  int height;
  int channels;
} image_data_t;

static int image_load(image_data_t *img, const char *path) {
  img->data = stbi_load(path, &img->width, &img->height, &img->channels, 4);
  if (!img->data) {
    return -1;
  }
  return 0;
}

static void image_free(image_data_t *img) {
  if (img->data) {
    stbi_image_free(img->data);
    img->data = NULL;
  }
}

static int image_render_to_buffer(image_data_t *img, struct shm_buffer *buffer,
                                  int dst_width, int dst_height) {
  uint8_t *src_data = img->data;
  uint8_t *data = buffer->data;
  int src_width = img->width;
  int src_height = img->height;

  int row_stride = src_width * 4;
  int col_stride = 4;

  for (int i = 0; i < src_width; i++) {
    for (int j = 0; j < src_height; j++) {
      int idx = j * row_stride + i * col_stride;
      data[idx] = src_data[idx + 2];
      data[idx + 1] = src_data[idx + 1];
      data[idx + 2] = src_data[idx];
      data[idx + 3] = src_data[idx + 3];
    }
  }

  return 0;
}

static void image_handler_destroy(wallpaper_handler_t *handler) {
  if (!handler) {
    return;
  }
  image_data_t *img = (image_data_t *)handler->data;
  if (img) {
    image_free(img);
    free(img);
  }
}

static int image_handler_render(wallpaper_handler_t *handler,
                                struct output_info *output_info) {
  if (!handler || !output_info) {
    return -1;
  }

  image_data_t *img = (image_data_t *)handler->data;
  if (!img || !img->data) {
    return -1;
  }

  while (!output_info->is_configured) {
    wl_display_dispatch(handler->state->display);
  }

  if (handler->total_outputs == 0) {
    handler->total_outputs = handler->state->output_count;
  }

  if (!handler->rendered_outputs) {
    output_info->src_width = img->width;
    output_info->src_height = img->height;
    wl_shm_pool_init(handler->state, output_info, 1);

    int dst_width = output_info->width;
    int dst_height = output_info->height;
    int src_width = img->width;
    int src_height = img->height;

    double scale_w = (double)dst_width / src_width;
    double scale_h = (double)dst_height / src_height;
    double scale = (scale_w > scale_h) ? scale_w : scale_h;

    double src_w = dst_width / scale;
    double src_h = dst_height / scale;
    double src_x = (src_width - src_w) / 2.0;
    double src_y = (src_height - src_h) / 2.0;

    wp_viewport_set_source(output_info->viewport, wl_fixed_from_double(src_x),
                           wl_fixed_from_double(src_y),
                           wl_fixed_from_double(src_w),
                           wl_fixed_from_double(src_h));
    wp_viewport_set_destination(output_info->viewport, dst_width, dst_height);
  }

  struct shm_buffer *buffer = get_shm_buffer(output_info->shm_pool);
  image_render_to_buffer(img, buffer, output_info->width, output_info->height);

  wl_surface_attach(output_info->surface, buffer->buffer, 0, 0);
  wl_surface_damage_buffer(output_info->surface, 0, 0, img->width, img->height);
  wl_surface_commit(output_info->surface);

  shm_pool_unmap_buffers(output_info->shm_pool);

  handler->rendered_outputs++;
  if (handler->rendered_outputs >= handler->total_outputs && img->data) {
    stbi_image_free(img->data);
    img->data = NULL;
  }

  return 0;
}

int image_handler_init(wallpaper_handler_t *handler, const char *path) {
  image_data_t *img = calloc(1, sizeof(image_data_t));
  if (!img) {
    return -1;
  }

  if (image_load(img, path) != 0) {
    free(img);
    return -1;
  }

  handler->data = img;
  handler->src_width = img->width;
  handler->src_height = img->height;
  handler->render = image_handler_render;
  handler->destroy = image_handler_destroy;

  return 0;
}
