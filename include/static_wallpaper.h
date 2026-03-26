#ifndef STATIC_WALLPAPER_H
#define STATIC_WALLPAPER_H

#include "wayland_state.h"
struct static_wallpaper {
    char* path;
    struct wayland_state* state;
    int src_width;
    int src_height;
    int src_channels;
    uint8_t* src_data;
    int render_init;
    int rendered_outputs;  // Number of outputs already rendered
    int total_outputs;     // Total number of outputs to render
};

struct static_wallpaper* create_static_wallpaper(char* path, struct wayland_state* state);

int render(struct static_wallpaper* wallpaper, struct output_info* output_info, int cpu_or_gpu);

int render_with_cpu(struct static_wallpaper* wallpaper, struct output_info* output_info);

int render_with_gpu(struct static_wallpaper* wallpaper, struct output_info* output_info);

void destroy_static_wallpaper(struct static_wallpaper* wallpaper);

#endif
