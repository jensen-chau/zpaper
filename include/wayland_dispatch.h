#ifndef WAYLAND_DISPATCH
#define WAYLAND_DISPATCH
#include "wlr-layer-shell-unstable-v1-protocol.h"
#include <wayland-client-protocol.h>
#include "linux-dmabuf-v1-protocol.h"

extern struct wl_registry_listener wl_registry_listener;
extern struct zwlr_layer_surface_v1_listener zwlr_layer_surface_v1_listener;
extern struct wl_buffer_listener wl_buffer_listener;
extern struct wl_output_listener wl_output_listener;
extern struct wl_callback_listener wl_callback_listener;
extern struct zwp_linux_buffer_params_v1_listener zwp_linux_buffer_params_v1_listener;
extern struct zwp_linux_dmabuf_feedback_v1_listener zwp_linux_dmabuf_feedback_v1_listener;
extern struct zwp_linux_dmabuf_v1_listener zwp_linux_dmabuf_v1_listener;

#endif
