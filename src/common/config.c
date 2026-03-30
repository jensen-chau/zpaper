#define STB_JSON_IMPLEMENTATION
#include "config.h"
#include "stb_json.h"
#include "utils.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char *safe_strdup(const char *str) {
  if (!str) {
    return NULL;
  }
  char *dup = malloc(strlen(str) + 1);
  if (dup) {
    strcpy(dup, str);
  }
  return dup;
}

static int ensure_config_dir(const char *path) {
  char dir[512];
  strncpy(dir, path, sizeof(dir) - 1);
  dir[sizeof(dir) - 1] = '\0';

  char *last_slash = strrchr(dir, '/');
  if (last_slash) {
    *last_slash = '\0';
    struct stat st;
    if (stat(dir, &st) == -1) {
      if (errno == ENOENT) {
        if (mkdir(dir, 0755) == -1) {
          return -1;
        }
      } else {
        return -1;
      }
    }
  }
  return 0;
}

const char *config_get_default_path(void) {
  static char path[512];
  const char *xdg_config = getenv("XDG_CONFIG_HOME");
  if (xdg_config) {
    snprintf(path, sizeof(path), "%s/zpaper/config.json", xdg_config);
  } else {
    const char *home = getenv("HOME");
    if (!home) {
      home = "/tmp";
    }
    snprintf(path, sizeof(path), "%s/.config/zpaper/config.json", home);
  }
  return path;
}

void config_init(zpaper_config_t *config) {
  memset(config, 0, sizeof(*config));
}

void config_free(zpaper_config_t *config) {
  if (!config) {
    return;
  }
  free(config->default_wallpaper);
  free(config->config_path);
  for (int i = 0; i < config->output_count; i++) {
    free(config->outputs[i].name);
    free(config->outputs[i].wallpaper_path);
  }
  config->output_count = 0;
  config->default_wallpaper = NULL;
  config->config_path = NULL;
}

static void config_defaults(zpaper_config_t *config) {
  config_free(config);
  config_init(config);
}

int config_load(const char *path, zpaper_config_t *config) {
  if (!path || !config) {
    return -1;
  }

  config_defaults(config);

  FILE *f = fopen(path, "r");
  if (!f) {
    return -1;
  }

  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (len <= 0) {
    fclose(f);
    return -1;
  }

  char *buffer = malloc(len + 1);
  if (!buffer) {
    fclose(f);
    return -1;
  }

  size_t read_len = fread(buffer, 1, len, f);
  buffer[read_len] = '\0';
  fclose(f);

  stbj_cursor cursor = stbj_load_buffer(buffer, read_len + 1);
  if (stbj_any_error(&cursor)) {
    free(buffer);
    return -1;
  }

  char path_buf[MAX_PATH_LEN];
  if (stbj_read_string_name(&cursor, "default", path_buf, sizeof(path_buf),
                            "") > 0 &&
      path_buf[0] != '\0') {
    config->default_wallpaper = safe_strdup(path_buf);
  }

  stbj_cursor outputs_cursor = stbj_move_cursor_name(&cursor, "outputs");
  if (!stbj_any_error(&outputs_cursor) && outputs_cursor.type == STBJ_OBJECT) {
    int count = stbj_count_values(&outputs_cursor);
    config->output_count = (count > MAX_OUTPUTS) ? MAX_OUTPUTS : count;

    for (int i = 0; i < config->output_count; i++) {
      stbj_cursor entry = stbj_move_cursor_index(&outputs_cursor, i);
      if (stbj_any_error(&entry)) {
        continue;
      }

      const char *name = stbj_find_index(&entry, 0);
      if (name) {
        config->outputs[i].name = safe_strdup(name);
      }

      if (stbj_count_values(&entry) > 1) {
        stbj_cursor value_cursor = stbj_move_cursor_index(&entry, 1);
        if (!stbj_any_error(&value_cursor) &&
            value_cursor.type == STBJ_OBJECT) {
          if (stbj_read_string_name(&value_cursor, "wallpaper", path_buf,
                                    sizeof(path_buf), "") > 0) {
            config->outputs[i].wallpaper_path = safe_strdup(path_buf);
          }
        }
      }
    }
  }

  config->config_path = safe_strdup(path);
  free(buffer);
  return 0;
}

int config_save(const char *path, const zpaper_config_t *config) {
  if (!path || !config) {
    return -1;
  }

  if (ensure_config_dir(path) != 0) {
    return -1;
  }

  FILE *f = fopen(path, "w");
  if (!f) {
    return -1;
  }

  fprintf(f, "{\n");
  fprintf(f, "  \"default\": \"%s\"\n",
          config->default_wallpaper ? config->default_wallpaper : "");
  fprintf(f, ",\n");
  fprintf(f, "  \"outputs\": {\n");

  for (int i = 0; i < config->output_count; i++) {
    fprintf(f, "    \"%s\": {\n",
            config->outputs[i].name ? config->outputs[i].name : "unknown");
    fprintf(f, "      \"wallpaper\": \"%s\"\n",
            config->outputs[i].wallpaper_path
                ? config->outputs[i].wallpaper_path
                : "");
    fprintf(f, "    }%s\n", (i < config->output_count - 1) ? "," : "");
  }

  fprintf(f, "  }\n");
  fprintf(f, "}\n");

  fclose(f);
  return 0;
}

int config_set_wallpaper(zpaper_config_t *config, const char *output_name,
                         const char *path) {
  if (!config || !path) {
    return -1;
  }

  if (strcmp(output_name, "default") == 0 || output_name[0] == '\0') {
    free(config->default_wallpaper);
    config->default_wallpaper = safe_strdup(path);
    for (int i = 0; i < config->output_count; i++) {
      free(config->outputs[i].name);
      free(config->outputs[i].wallpaper_path);
      config->outputs[i].name = NULL;
      config->outputs[i].wallpaper_path = NULL;
    }
    config->output_count = 0;
    return 0;
  }

  for (int i = 0; i < config->output_count; i++) {
    if (config->outputs[i].name &&
        strcmp(config->outputs[i].name, output_name) == 0) {
      free(config->outputs[i].wallpaper_path);
      config->outputs[i].wallpaper_path = safe_strdup(path);
      free(config->default_wallpaper);
      config->default_wallpaper = safe_strdup(path);
      return 0;
    }
  }

  if (config->output_count >= MAX_OUTPUTS) {
    return -1;
  }

  int idx = config->output_count++;
  config->outputs[idx].name = safe_strdup(output_name);
  config->outputs[idx].wallpaper_path = safe_strdup(path);
  free(config->default_wallpaper);
  config->default_wallpaper = safe_strdup(path);
  return 0;
}

const char *config_get_wallpaper(const zpaper_config_t *config,
                                 const char *output_name) {
  if (!config || !output_name) {
    return NULL;
  }

  for (int i = 0; i < config->output_count; i++) {
    if (config->outputs[i].name &&
        strcmp(config->outputs[i].name, output_name) == 0) {
      return config->outputs[i].wallpaper_path;
    }
  }

  return config->default_wallpaper;
}
