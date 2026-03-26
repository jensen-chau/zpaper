#define _GNU_SOURCE
#include "wl_shm_pool.h"
#include "utils.h"
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include "wayland_state.h"

int create_shm_buffer(struct shm_pool *shm_pool) {
  for (int i = 0; i < shm_pool->buffer_count; i++) {
    shm_pool->shm_buffers[i] =
        (struct shm_buffer *)malloc(sizeof(struct shm_buffer));

    if (!shm_pool->shm_buffers[i]) {
      ERR("malloc shm_buffer error");
      return 1;
    }

    shm_pool->shm_buffers[i]->buffer = wl_shm_pool_create_buffer(
        shm_pool->wl_shm_pool,
        shm_pool->ssize * i,
        shm_pool->buffer_width, shm_pool->buffer_height,
        shm_pool->buffer_width * sizeof(int), WL_SHM_FORMAT_ARGB8888);

    if (!shm_pool->shm_buffers[i]->buffer) {
        ERR("create buffer error");
        return 1;
    }

    shm_pool->shm_buffers[i]->busy = 0;
    void* ret = mmap(NULL, shm_pool->ssize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_pool->fd, shm_pool->ssize * i);
    if (ret == MAP_FAILED) {
      ERR("mmap error");
      return 1;
    }
    shm_pool->shm_buffers[i]->data = ret;
  }

  return 0;
}

int wl_shm_pool_init(struct wayland_state *state,
                     struct output_info *output_info, int buffer_count) {
  LOG("begine shm pool init");
  struct shm_pool *shm_pool =
      (struct shm_pool *)malloc(sizeof(struct shm_pool));
  if (!shm_pool) {
    ERR("malloc shm_pool error");
    return 1;
  }

  shm_pool->shm_buffers =
      (struct shm_buffer **)malloc(sizeof(struct shm_buffer *) * buffer_count);
  if (!shm_pool->shm_buffers) {
    ERR("malloc shm_buffers error");
    free(shm_pool);
    return 1;
  }

  // Initialize shm_buffers to NULL
  for (int i = 0; i < buffer_count; i++) {
    shm_pool->shm_buffers[i] = NULL;
  }

  shm_pool->buffer_count = buffer_count;
  shm_pool->buffer_width = output_info->src_width;
  shm_pool->buffer_height = output_info->src_height;

  struct wl_shm *shm = state->shm;
  int64_t pool_size = output_info->src_width * output_info->src_height * sizeof(int) * buffer_count;

  shm_pool->ssize = output_info->src_width * output_info->src_height * sizeof(int);
  int fd = memfd_create("zpaper-shm-pool", MFD_CLOEXEC);
  if (fd < 0) {
    ERR("memfd_create error");
    free(shm_pool->shm_buffers);
    free(shm_pool);
    return 1;
  }

  if (ftruncate(fd, pool_size) < 0) {
    ERR("ftruncate error");
    close(fd);
    free(shm_pool->shm_buffers);
    free(shm_pool);
    return 1;
  }

  shm_pool->wl_shm_pool = wl_shm_create_pool(shm, fd, pool_size);
  if (!shm_pool->wl_shm_pool) {
    ERR("create shm pool error");
    close(fd);
    free(shm_pool->shm_buffers);
    free(shm_pool);
    return 1;
  }
  shm_pool->fd = fd;

  output_info->shm_pool = shm_pool;

  int ret = create_shm_buffer(shm_pool);
  if (ret) {
    ERR("create shm buffer error");
    // Cleanup partially created buffers
    if (shm_pool->shm_buffers) {
      for (int i = 0; i < buffer_count; i++) {
        if (shm_pool->shm_buffers[i]) {
          if (shm_pool->shm_buffers[i]->data) {
            munmap(shm_pool->shm_buffers[i]->data, shm_pool->ssize);
          }
          if (shm_pool->shm_buffers[i]->buffer) {
            wl_buffer_destroy(shm_pool->shm_buffers[i]->buffer);
          }
          free(shm_pool->shm_buffers[i]);
        }
      }
      free(shm_pool->shm_buffers);
    }
    if (shm_pool->wl_shm_pool) {
      wl_shm_pool_destroy(shm_pool->wl_shm_pool);
    }
    close(fd);
    free(shm_pool);
    return 1;
  }
  LOG("shm pool init successful");
  return 0;
}
struct shm_buffer *get_shm_buffer(struct shm_pool* pool) {
    if (!pool) {
        ERR("pool is null");
        return NULL;
    }
    for (int i = 0; i < pool->buffer_count; i++) {
        if (!pool->shm_buffers[i]->busy) {
            LOG("return buffer %d, width: %d, height: %d", i, pool->buffer_width, pool->buffer_height);
            return pool->shm_buffers[i];
        }
    }
    ERR("All buffer busy");
    return NULL;
}

int mark_busy(struct shm_pool* pool, struct wl_buffer *buffer);

int mark_idle(struct shm_pool* pool, struct wl_buffer *buffer);

void shm_pool_unmap_buffers(struct shm_pool* pool) {
  if (!pool || !pool->shm_buffers) {
    return;
  }

  for (int i = 0; i < pool->buffer_count; i++) {
    struct shm_buffer *buf = pool->shm_buffers[i];
    if (buf && buf->data) {
      munmap(buf->data, pool->ssize);
      buf->data = NULL;
    }
  }
}

void shm_pool_destroy(struct shm_pool* pool) {
  if (!pool) {
    return;
  }

  // Destroy all buffers and unmap memory
  if (pool->shm_buffers) {
    for (int i = 0; i < pool->buffer_count; i++) {
      struct shm_buffer *buf = pool->shm_buffers[i];
      if (buf) {
        // Unmap memory
        if (buf->data) {
          munmap(buf->data, pool->ssize);
          buf->data = NULL;
        }
        // Destroy wl_buffer
        if (buf->buffer) {
          wl_buffer_destroy(buf->buffer);
          buf->buffer = NULL;
        }
        free(buf);
        pool->shm_buffers[i] = NULL;
      }
    }
    free(pool->shm_buffers);
    pool->shm_buffers = NULL;
  }

  // Destroy wl_shm_pool
  if (pool->wl_shm_pool) {
    wl_shm_pool_destroy(pool->wl_shm_pool);
    pool->wl_shm_pool = NULL;
  }

  // Close file descriptor
  if (pool->fd >= 0) {
    close(pool->fd);
    pool->fd = -1;
  }

  // Free the pool structure
  free(pool);
}

