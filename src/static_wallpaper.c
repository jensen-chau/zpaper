#include "viewporter-protocol.h"
#include <stdint.h>
#include <wayland-client-protocol.h>
#define STB_IMAGE_IMPLEMENTATION
#include "static_wallpaper.h"
#include <stdlib.h>
#include "stb_image.h"
#include "utils.h"
#include "wayland_state.h"
#include "wl_shm_pool.h"

struct static_wallpaper* create_static_wallpaper(char* path, struct wayland_state* state) {
    struct static_wallpaper* wallpaper = malloc(sizeof(struct static_wallpaper));
    wallpaper->path = path;
    stbi_uc* ima_data = stbi_load(path, &wallpaper->src_width, &wallpaper->src_height, &wallpaper->src_channels, 4);
    wallpaper->src_data = ima_data;
    wallpaper->render_init = 0;
    wallpaper->state = state;
    return wallpaper;
}


int render(struct static_wallpaper *wallpaper, struct output_info *output_info, int cpu_or_gpu) {
    int ret = 0;
    if (cpu_or_gpu == 0) {
        ret = render_with_cpu(wallpaper, output_info);
    } else {
        ret = render_with_gpu(wallpaper, output_info);
    }
    return ret;
}

int render_with_cpu_init(struct static_wallpaper *wallpaper, struct output_info *output_info) {
    output_info->src_width = wallpaper->src_width;
    output_info->src_height = wallpaper->src_height;
    wl_shm_pool_init(wallpaper->state, output_info, 2);
    return 0;
}

int render_with_cpu(struct static_wallpaper *wallpaper, struct output_info *output_info) {
    if (!wallpaper->render_init) {
        wallpaper->render_init = 1;
        render_with_cpu_init(wallpaper, output_info);
    }

    struct shm_buffer *buffer = get_shm_buffer(output_info->shm_pool);
    uint8_t* src_data = wallpaper->src_data;
    uint8_t* data = buffer->data;
    int src_width = wallpaper->src_width;
    int src_height = wallpaper->src_height;
    int dst_width = output_info->width;
    int dst_height = output_info->height;

    int row_stride = src_width * 4;
    int col_stride = 4;

    //copy data from wallpaper to buffer
    for(int i = 0; i < src_width; i++) {
        for(int j = 0; j < src_height; j++) {
            int idx = j * row_stride + i * col_stride;
             data[idx] = src_data[idx];
             data[idx+1] = src_data[idx+1];
             data[idx+2] = src_data[idx+2];
             data[idx+3] = src_data[idx+3];
        }
    }

    wp_viewport_set_destination(output_info->viewport, dst_width, dst_height);

    wl_surface_attach(output_info->surface, buffer->buffer, 0, 0);
    wl_surface_damage_buffer(output_info->surface, 0, 0, src_width, src_height);
    wl_surface_commit(output_info->surface);

    LOG("render success");

    return 0;
}

int render_with_gpu(struct static_wallpaper *wallpaper, struct output_info *output_info) {
    ERR("no implement for gpu render");
    return 1;
}
