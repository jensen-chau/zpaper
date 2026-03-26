#include "static_wallpaper.h"
#include "wayland_state.h"
#include <unistd.h>

#define WALLPAPER_PATH "/home/zjx/Pictures/wallpaper/02.png"

static struct wayland_state *global_state = NULL;
static struct static_wallpaper *global_wallpaper = NULL;

void signal_handler(int signum) {
  (void)signum;
  if (global_state && global_state->display) {
    wl_display_cancel_read(global_state->display);
  }
}

void cleanup() {
  if (global_wallpaper) {
    destroy_static_wallpaper(global_wallpaper);
    global_wallpaper = NULL;
  }

  if (global_state) {
    for (int i = 0; i < global_state->output_count; i++) {
      if (global_state->output_infos[i] && global_state->output_infos[i]->shm_pool) {
        shm_pool_destroy(global_state->output_infos[i]->shm_pool);
        global_state->output_infos[i]->shm_pool = NULL;
      }
    }

    global_state = NULL;
  }
}

int main() {

  global_state = wayland_state_new();

  global_wallpaper = create_static_wallpaper(WALLPAPER_PATH, global_state);

  while (1) {
    int all_configured = 1;
    for (int i = 0; i < global_state->output_count; i++) {
      if (!global_state->output_infos[i]->is_configured) {
        all_configured = 0;
        break;
      }
    }
    if (all_configured) {
      break;
    }
    if (wl_display_dispatch(global_state->display) == -1) {
      break;
    }
  }

  for (int i = 0; i < global_state->output_count; i++) {
    render(global_wallpaper, global_state->output_infos[i], 0);
  }

  while (wl_display_dispatch(global_state->display) != -1) {
  }

  cleanup();

  return 0;
}
