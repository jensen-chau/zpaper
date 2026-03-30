#ifndef ZPAPER_CONFIG_H
#define ZPAPER_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#define MAX_OUTPUTS 16
#define MAX_PATH_LEN 512

typedef struct {
  char *name;
  char *wallpaper_path;
} output_config_t;

typedef struct {
  char *default_wallpaper;
  output_config_t outputs[MAX_OUTPUTS];
  int output_count;
  char *config_path;
} zpaper_config_t;

int config_load(const char *path, zpaper_config_t *config);
int config_save(const char *path, const zpaper_config_t *config);
int config_set_wallpaper(zpaper_config_t *config, const char *output_name,
                         const char *path);
const char *config_get_wallpaper(const zpaper_config_t *config,
                                 const char *output_name);
void config_free(zpaper_config_t *config);
const char *config_get_default_path(void);
void config_init(zpaper_config_t *config);

#endif
