#ifndef WAYLAND_STATE
#define WAYLAND_STATE
#include <wayland-client-protocol.h>
#include <wayland-client.h>

struct wayland_state {
    struct wl_display *display;
    struct wl_compositor *compositor;
};

#endif
