#include "static_wallpaper.h"
#include "utils.h"
#include "wayland_state.h"
#include <stdio.h>

#define WALLPAPER_PATH "/home/zjx/Pictures/wallpaper/01.jpg"

int main() {
    struct wayland_state* state = wayland_state_new();
    UNUSE(state);

    struct static_wallpaper* wallpaper = create_static_wallpaper(WALLPAPER_PATH, state);

    for (int i = 0; i < state->output_count; i++) {
        render(wallpaper, state->output_infos[i], 0);
    }

    while (wl_display_dispatch(state->display) != -1) {
    
    }

    printf("hello world!\n");
}
