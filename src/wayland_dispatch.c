#include "wayland_dispatch.h"
#include "utils.h"
#include "wayland_state.h"
#include <wayland-client-protocol.h>
#include <string.h>
#include <stdlib.h>
#include <wayland-client.h>
// ================ wl_register_listener ===================

void global(void *data, struct wl_registry *wl_registry, uint32_t name,
            const char *interface, uint32_t version) {
    struct wayland_state* state = (struct wayland_state*)data;
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        state->compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, version);
        LOG("wl_compositor_interface binded");
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        state->shm = wl_registry_bind(wl_registry, name, &wl_shm_interface, version);
        LOG("wl_shm_interface binded");
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        state->layer_shell = wl_registry_bind(wl_registry, name, &zwlr_layer_shell_v1_interface, version);
        LOG("zwlr_layer_shell_v1_interface binded");
    } else if (strcmp(interface, zwp_linux_dmabuf_v1_interface.name) == 0) {
        state->dmabuf = wl_registry_bind(wl_registry, name, &zwp_linux_dmabuf_v1_interface, version);
        LOG("zwp_linux_dmabuf_v1_interface binded");
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        struct output_info* output_info = (struct output_info*)malloc(sizeof(struct output_info));
        output_info->name = NULL;
        output_info->description = NULL;
        output_info->scale = 0;
        output_info->output = wl_registry_bind(wl_registry, name, &wl_output_interface, version);
        state->output_infos = realloc(state->output_infos, sizeof(struct output_info*)*(state->output_count+1));
        state->output_count++;
        state->output_infos[state->output_count-1] = output_info;
        LOG("find output, current_output_count: %d", state->output_count);
    }
}

void global_remove(void *data, struct wl_registry *wl_registry, uint32_t name) {
}

struct wl_registry_listener wl_registry_listener = {global, global_remove};
