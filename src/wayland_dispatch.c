#include "wayland_dispatch.h"
#include "utils.h"
#include "viewporter-protocol.h"
#include "wayland_state.h"
#include "wlr-layer-shell-unstable-v1-protocol.h"
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
// ================ wl_register_listener ===================

void global(void *data, struct wl_registry *wl_registry, uint32_t name,
            const char *interface, uint32_t version) {
  struct wayland_state *state = (struct wayland_state *)data;
  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    state->compositor =
        wl_registry_bind(wl_registry, name, &wl_compositor_interface, version);
    LOG("wl_compositor_interface binded");
  } else if (strcmp(interface, wl_shm_interface.name) == 0) {
    state->shm =
        wl_registry_bind(wl_registry, name, &wl_shm_interface, version);
    LOG("wl_shm_interface binded");
  } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
    state->layer_shell = wl_registry_bind(
        wl_registry, name, &zwlr_layer_shell_v1_interface, version);
    LOG("zwlr_layer_shell_v1_interface binded");
  } else if (strcmp(interface, zwp_linux_dmabuf_v1_interface.name) == 0) {
    state->dmabuf = wl_registry_bind(wl_registry, name,
                                     &zwp_linux_dmabuf_v1_interface, version);
    LOG("zwp_linux_dmabuf_v1_interface binded");
  } else if (strcmp(interface, wl_output_interface.name) == 0) {
    struct output_info *output_info =
        (struct output_info *)malloc(sizeof(struct output_info));
    output_info->name = NULL;
    output_info->description = NULL;
    output_info->scale = 0;
    output_info->output =
        wl_registry_bind(wl_registry, name, &wl_output_interface, version);
    wl_output_add_listener(output_info->output, &wl_output_listener, state);
    state->output_infos =
        realloc(state->output_infos,
                sizeof(struct output_info *) * (state->output_count + 1));
    state->output_count++;
    state->output_infos[state->output_count - 1] = output_info;
    LOG("find output, current_output_count: %d", state->output_count);
  } else if (strcmp(wp_viewporter_interface.name, interface) == 0) {
    state->viewporter =
        wl_registry_bind(wl_registry, name, &wp_viewporter_interface, version);
    LOG("wp_viewporter_interface binded");
  
  }
}

void global_remove(void *data, struct wl_registry *wl_registry, uint32_t name) {
}

struct wl_registry_listener wl_registry_listener = {
    .global = global, .global_remove = global_remove};

// ================ zwlr_layer_surface_v1_listener ====================

void configure(void *data, struct zwlr_layer_surface_v1 *zwlr_layer_surface_v1,
               uint32_t serial, uint32_t width, uint32_t height) {
  struct wayland_state *state = (struct wayland_state *)data;
  for (int i = 0; i < state->output_count; i++) {
    if (state->output_infos[i]->layer_surface == zwlr_layer_surface_v1) {
      state->output_infos[i]->is_configured = 1;
      state->output_infos[i]->width = width;
      state->output_infos[i]->height = height;
      LOG("output: %d configure, width: %d, height: %d", i, width, height);
      zwlr_layer_surface_v1_ack_configure(zwlr_layer_surface_v1, serial);
    }
  }
}
void closed(void *data, struct zwlr_layer_surface_v1 *zwlr_layer_surface_v1) {}

struct zwlr_layer_surface_v1_listener zwlr_layer_surface_v1_listener = {
    .configure = configure, .closed = closed};

// ================ wl_output_listener ==================================

void geometry(void *data, struct wl_output *wl_output, int32_t x, int32_t y,
              int32_t physical_width, int32_t physical_height, int32_t subpixel,
              const char *make, const char *model, int32_t transform) {
    struct wayland_state* state = (struct wayland_state*)data;
    for (int i = 0; i < state->output_count; i++) {
        if (state->output_infos[i]->output == wl_output) {
        
        }
    }
}

void mode(void *data, struct wl_output *wl_output, uint32_t flags,
          int32_t width, int32_t height, int32_t refresh) {
    struct wayland_state* state = (struct wayland_state*)data;
    for (int i = 0; i < state->output_count; i++) {
        if (state->output_infos[i]->output == wl_output) {
            state->output_infos[i]->width = width;
            state->output_infos[i]->height = height;
            LOG("wl_output mode event: %dx%d", width, height);
            return;
        }
    }
}
void done(void *data, struct wl_output *wl_output) {}
void scale(void *data, struct wl_output *wl_output, int32_t factor) {}
void name(void *data, struct wl_output *wl_output, const char *name) {
    struct wayland_state* state = (struct wayland_state*)data;
    for (int i = 0; i < state->output_count; i++) {
        if (state->output_infos[i]->output == wl_output) {
            state->output_infos[i]->name = strdup(name);   
            LOG("wl_output name event: %s", name);
            return;
        }
    }
}
void description(void *data, struct wl_output *wl_output,
                 const char *description) {
    struct wayland_state* state = (struct wayland_state*)data;
    for (int i = 0; i < state->output_count; i++) {
        if (state->output_infos[i]->output == wl_output) {
            state->output_infos[i]->description = strdup(description);
            LOG("wl_output description event: %s", description);
        }
    }
}
struct wl_output_listener wl_output_listener = {
    .done = done,
    .description = description,
    .geometry = geometry,
    .name = name,
    .scale = scale,
    .mode = mode,
};
