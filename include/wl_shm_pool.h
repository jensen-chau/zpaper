#ifndef WL_SHM_POOL_H
#define WL_SHM_POOL_H

//#include "wayland_state.h"
struct wayland_state;
struct output_info;

#include <stdint.h>
#include <wayland-client-protocol.h>
struct shm_buffer {
  struct wl_buffer *buffer;
  int busy;
  uint8_t* data;
};

struct shm_pool {
  struct shm_buffer **shm_buffers;
  struct wl_shm_pool *wl_shm_pool;
  int buffer_count;
  int buffer_width;
  int buffer_height;
  int64_t ssize; 
  int fd;
};

int wl_shm_pool_init(struct wayland_state* state, struct output_info* output_info, int buffer_count);

struct shm_buffer *get_shm_buffer(struct shm_pool* pool);

int mark_busy(struct shm_pool* pool, struct wl_buffer *buffer);

int mark_idle(struct shm_pool* pool, struct wl_buffer *buffer);

void shm_pool_destroy(struct shm_pool* pool);

void shm_pool_unmap_buffers(struct shm_pool* pool);

#endif
