#include "viewporter-protocol.h"
#include <stdint.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>
#define STB_IMAGE_IMPLEMENTATION
#include "static_wallpaper.h"
#include "stb_image.h"
#include "utils.h"
#include "wayland_state.h"
#include "wl_shm_pool.h"
#include <stdlib.h>

struct static_wallpaper *create_static_wallpaper(char *path,
                                                 struct wayland_state *state) {
  struct static_wallpaper *wallpaper = malloc(sizeof(struct static_wallpaper));
  wallpaper->path = path;
  stbi_uc *ima_data =
      stbi_load(path, &wallpaper->src_width, &wallpaper->src_height,
                &wallpaper->src_channels, 4);
  wallpaper->src_data = ima_data;
  wallpaper->render_init = 0;
  wallpaper->state = state;
  wallpaper->rendered_outputs = 0;
  wallpaper->total_outputs = 0;
  return wallpaper;
}

int render(struct static_wallpaper *wallpaper, struct output_info *output_info,
           int cpu_or_gpu) {
  // Set total outputs on first call
  if (wallpaper->total_outputs == 0) {
    wallpaper->total_outputs = wallpaper->state->output_count;
  }

  int ret = 0;
  if (cpu_or_gpu == 0) {
    ret = render_with_cpu(wallpaper, output_info);
  } else {
    ret = render_with_gpu(wallpaper, output_info);
  }
  return ret;
}

int render_with_cpu_init(struct static_wallpaper *wallpaper,
                         struct output_info *output_info) {
  output_info->src_width = wallpaper->src_width;
  output_info->src_height = wallpaper->src_height;
  // Static wallpaper only needs 1 buffer
  wl_shm_pool_init(wallpaper->state, output_info, 1);

  int dst_width = output_info->width;
  int dst_height = output_info->height;
  int src_width = wallpaper->src_width;
  int src_height = wallpaper->src_height;

  // Calculate cover mode: scale image to fill the entire screen while
  // maintaining aspect ratio
  double scale_w = (double)dst_width / src_width;
  double scale_h = (double)dst_height / src_height;
  double scale = (scale_w > scale_h) ? scale_w : scale_h;

  // Calculate source rectangle in buffer coordinates
  double src_w = dst_width / scale;
  double src_h = dst_height / scale;
  double src_x = (src_width - src_w) / 2.0;
  double src_y = (src_height - src_h) / 2.0;

  // Set source rectangle (crop) - this tells Wayland which part of the image to
  // display
  wp_viewport_set_source(output_info->viewport, wl_fixed_from_double(src_x),
                         wl_fixed_from_double(src_y),
                         wl_fixed_from_double(src_w),
                         wl_fixed_from_double(src_h));
  // Set destination rectangle - this tells Wayland where to display on the
  // output
  wp_viewport_set_destination(output_info->viewport, dst_width, dst_height);

  return 0;
}

int render_with_cpu(struct static_wallpaper *wallpaper,
                    struct output_info *output_info) {
  if (!wallpaper->render_init) {
    wallpaper->render_init = 1;
    render_with_cpu_init(wallpaper, output_info);
  }

  struct shm_buffer *buffer = get_shm_buffer(output_info->shm_pool);

  if (wallpaper->src_data) {
    uint8_t *src_data = wallpaper->src_data;
    uint8_t *data = buffer->data;
    int src_width = wallpaper->src_width;
    int src_height = wallpaper->src_height;

    int row_stride = src_width * 4;
    int col_stride = 4;

    // copy data from wallpaper to buffer, convert RGBA to BGRA (ARGB8888 on
    // little-endian)
    for (int i = 0; i < src_width; i++) {
      for (int j = 0; j < src_height; j++) {
        int idx = j * row_stride + i * col_stride;
        // stb_image returns RGBA, Wayland SHM on little-endian needs BGRA
        // src: R(idx), G(idx+1), B(idx+2), A(idx+3)
        // dst: B(idx), G(idx+1), R(idx+2), A(idx+3)
        data[idx] = src_data[idx + 2];     // Blue
        data[idx + 1] = src_data[idx + 1]; // Green
        data[idx + 2] = src_data[idx];     // Red
        data[idx + 3] = src_data[idx + 3]; // Alpha
      }
    }
  }

  wl_surface_attach(output_info->surface, buffer->buffer, 0, 0);
  wl_surface_damage_buffer(output_info->surface, 0, 0, wallpaper->src_width, wallpaper->src_height);
  wl_surface_commit(output_info->surface);

  // Unmap buffers after commit to free memory (static wallpaper doesn't need access after rendering)
  shm_pool_unmap_buffers(output_info->shm_pool);

  // Free source image data after all outputs are rendered
  wallpaper->rendered_outputs++;
  if (wallpaper->rendered_outputs >= wallpaper->total_outputs && wallpaper->src_data) {
    stbi_image_free(wallpaper->src_data);
    wallpaper->src_data = NULL;
  }

  return 0;
}

int render_with_gpu(struct static_wallpaper *wallpaper,
                    struct output_info *output_info) {
  ERR("no implement for gpu render");
  return 1;
}

void destroy_static_wallpaper(struct static_wallpaper *wallpaper) {
  if (!wallpaper) {
    return;
  }

  // Free stb_image data
  if (wallpaper->src_data) {
    stbi_image_free(wallpaper->src_data);
    wallpaper->src_data = NULL;
  }

  free(wallpaper);
}
