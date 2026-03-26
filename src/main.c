#include "static_wallpaper.h"
#include "utils.h"
#include "wayland_state.h"
#include <stdio.h>

#define WALLPAPER_PATH "/home/zjx/Pictures/wallpaper/02.png"

int main() {
    struct wayland_state* state = wayland_state_new();
    UNUSE(state);

    struct static_wallpaper* wallpaper = create_static_wallpaper(WALLPAPER_PATH, state);

    // Wait for all layer surfaces to be configured
    while (1) {
        int all_configured = 1;
        for (int i = 0; i < state->output_count; i++) {
            if (!state->output_infos[i]->is_configured) {
                all_configured = 0;
                break;
            }
        }
        if (all_configured) {
            break;
        }
        wl_display_dispatch(state->display);
    }

    for (int i = 0; i < state->output_count; i++) {
        render(wallpaper, state->output_infos[i], 0);
    }

    while (wl_display_dispatch(state->display) != -1) {

    }

    printf("hello world!\n");
}
