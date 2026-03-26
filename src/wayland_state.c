#include "wayland_state.h"
#include <stdlib.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include "wayland_dispatch.h"

struct wayland_state* wayland_state_new() {
    struct wayland_state* state = (struct wayland_state*)malloc(sizeof(struct wayland_state));
    state->display = wl_display_connect(NULL);
    struct wl_registry* registry = wl_display_get_registry(state->display);
    wl_registry_add_listener(registry, &wl_registry_listener, state);
    wl_display_roundtrip(state->display);
    return state;
}
