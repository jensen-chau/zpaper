#include "wayland_state.h"
#include "viewporter-protocol.h"
#include "wayland_dispatch.h"
#include "wlr-layer-shell-unstable-v1-protocol.h"
#include <stdlib.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

struct wayland_state *wayland_state_new() {
  struct wayland_state *state =
      (struct wayland_state *)calloc(1, sizeof(struct wayland_state));
  state->display = wl_display_connect(NULL);
  struct wl_registry *registry = wl_display_get_registry(state->display);
  wl_registry_add_listener(registry, &wl_registry_listener, state);
  wl_display_roundtrip(state->display);

  wl_display_roundtrip(state->display);
  create_layer_surface(state);

  wl_display_roundtrip(state->display);
  wl_display_roundtrip(state->display);

  return state;
}

int create_layer_surface(struct wayland_state *state) {
  for (int i = 0; i < state->output_count; i++) {
    struct output_info *info = state->output_infos[i];
    info->surface = wl_compositor_create_surface(state->compositor);

    info->viewport =
        wp_viewporter_get_viewport(state->viewporter, info->surface);

    info->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        state->layer_shell, info->surface, info->output,
        ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND, "zpaper");
    zwlr_layer_surface_v1_set_size(info->layer_surface, info->width,
                                   info->height);
    zwlr_layer_surface_v1_set_exclusive_zone(
        info->layer_surface, ZWLR_LAYER_SURFACE_V1_SET_EXCLUSIVE_ZONE);
    zwlr_layer_surface_v1_set_anchor(info->layer_surface,
                                     ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                         ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
                                         ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                         ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP);
    zwlr_layer_surface_v1_set_keyboard_interactivity(info->layer_surface, 0);

    zwlr_layer_surface_v1_add_listener(info->layer_surface,
                                       &zwlr_layer_surface_v1_listener, state);
    wl_surface_commit(info->surface);
  }
  return 0;
}
