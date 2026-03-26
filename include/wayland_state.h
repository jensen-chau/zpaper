#ifndef WAYLAND_STATE
#define WAYLAND_STATE
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include "wl_shm_pool.h"
#include "wlr-layer-shell-unstable-v1-protocol.h"
#include "linux-dmabuf-v1-protocol.h"
#include "viewporter-protocol.h"

struct output_info {
    int width;
    int height;
    int src_width;
    int src_height;
    struct wl_output* output;
    struct wl_surface* surface;
    struct wl_callback* wl_callback;
    struct wp_viewport* viewport;
    struct zwlr_layer_surface_v1* layer_surface;
    struct wl_buffer* buffer;
    struct shm_pool* shm_pool;
    int is_configured;
    char* name;
    char* description;
    int scale;
};

struct wayland_state {
    struct wl_display *display;
    struct wl_compositor *compositor;
    struct wp_viewporter *viewporter;
    struct zwlr_layer_shell_v1 *layer_shell;
    struct wl_shm *shm;
    struct zwp_linux_dmabuf_v1 *dmabuf;
    struct output_info **output_infos;
    int output_count;
};

struct wayland_state* wayland_state_new();
int create_layer_surface(struct wayland_state* state);

#endif
