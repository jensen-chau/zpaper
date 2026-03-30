# AGENTS.md - Zpaper Developer Guide

## Project Overview

Zpaper is a lightweight Wayland wallpaper application written in C. It renders static wallpapers on Wayland compositors using the layer-shell protocol.

## Build Commands

```bash
# Build the project
make

# Clean build artifacts (including generated protocol files)
make clean

# Install to system
make install

# Uninstall from system
make uninstall
```

**No test framework is currently configured for this project.**

## Project Structure

```
.
├── src/           # Source code (.c files)
├── include/       # Public headers (.h files)
├── protocol/      # Wayland protocol XML files
├── build/         # Build output (generated)
├── Makefile       # Build system
└── .clang-format # Code formatting rules (empty)
```

## Code Style Guidelines

### General Rules

- **2-space indentation** (no tabs)
- **Curly braces on same line** for function definitions: `int func() {`
- **Opening brace on new line** for control statements: `if (x) {`
- **Space after keywords**: `if (`, `for (`, `while (`
- **No space before parentheses** in function calls: `func(arg)`
- **Variable declarations**: pointers use `*` adjacent to variable name: `char *path`

### Naming Conventions

- **Functions**: `snake_case` (e.g., `wayland_state_new`, `create_layer_surface`)
- **Structs/Types**: `snake_case` with `_t` suffix for typedefs (e.g., `struct wayland_state`, `struct output_info`)
- **Constants/Macros**: `SCREAMING_SNAKE_CASE` (e.g., `WALLPAPER_PATH`)
- **Variables**: `snake_case` (e.g., `global_state`, `output_count`)
- **Header guards**: `HEADER_NAME_H` format (e.g., `WAYLAND_STATE_H`)

### Imports

- **Standard order** (top of file):
  1. Generated protocol headers (e.g., `"viewporter-protocol.h"`)
  2. Project headers (e.g., `"static_wallpaper.h"`)
  3. System headers (e.g., `<stdlib.h>`, `<wayland-client.h>`)

- Use `#include "..."` for local headers, `#include <...>` for system/library headers

### Error Handling

- Return `0` for success, non-zero for failure (following Unix conventions)
- Use `ERR()` macro from `utils.h` for error messages (e.g., `ERR("no implement for gpu render");`)
- Check return values from Wayland API calls
- Clean up resources in reverse order of allocation

### Memory Management

- Use `malloc()`/`free()` from `<stdlib.h>`
- Always check allocations for NULL (except where not needed per project style)
- Free all resources in cleanup functions
- Use `stbi_image_free()` for image data loaded via stb_image

### Wayland-Specific Patterns

- **Registry setup**: Connect display → get registry → add listener → roundtrip
- **Output handling**: Iterate `state->output_count` and `state->output_infos[i]`
- **SHM buffers**: Use `wl_shm_pool_*` functions from `wl_shm_pool.h`
- **Protocol listeners**: Add listeners with `*_add_listener()` after object creation

### Code Examples

**Function definition:**
```c
int create_layer_surface(struct wayland_state *state) {
  for (int i = 0; i < state->output_count; i++) {
    // ...
  }
  return 0;
}
```

**Struct definition:**
```c
struct wayland_state {
    struct wl_display *display;
    struct wl_compositor *compositor;
    int output_count;
};
```

**Header guard:**
```c
#ifndef WAYLAND_STATE_H
#define WAYLAND_STATE_H
// ...
#endif
```

### Linting/Formatting

The `.clang-format` file is currently empty. If adding formatting:

```bash
# Format a single file (requires clang-format to be installed)
clang-format -i src/wayland_state.c
```

**Manual formatting rules** (until `.clang-format` is configured):
- 2 spaces for indentation
- No tabs
- Max line length: 100 characters (soft limit)
- Trailing newlines at EOF

### Adding New Features

1. **New source file**: Add to `src/` and update Makefile if needed (wildcard picks up automatically)
2. **New protocol**: Add XML to `protocol/`, build generates `*-protocol.c` and `*-protocol.h`
3. **New header**: Add to `include/` for public API, or `src/` for internal use
4. **New dependency**: Update `LIBS` in Makefile

### Common Tasks

**Change wallpaper path**: Edit `WALLPAPER_PATH` in `src/main.c` (currently hardcoded)

**Add new output callback**: Implement in `wayland_dispatch.c` and register in `wayland_state.c`

**Debug Wayland issues**: Use `WAYLAND_DEBUG=1` environment variable

## Dependencies

- GCC or Clang
- Wayland client libraries (`libwayland-client`)
- `wayland-scanner` (for protocol code generation)
- `pkg-config`
- `stb_image.h` (included in repo, see `include/stb_image.h`)
