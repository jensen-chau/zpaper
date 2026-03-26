# Zpaper

A lightweight Wayland wallpaper application written in C.

## Features

- Static wallpaper support
- Multi-monitor support with independent wallpaper for each output
- Low memory footprint - releases image data after rendering
- Layer-shell protocol integration
- Cover mode scaling (fills screen while maintaining aspect ratio)

## Requirements

- Wayland compositor with layer-shell protocol support
- C compiler (GCC or Clang)
- Wayland development libraries
- pkg-config

## Building

```bash
make
```

## Usage

```bash
./zpaper
```

Currently, the wallpaper path is hardcoded in `src/main.c`. To change the wallpaper, modify the `WALLPAPER_PATH` constant:

```c
#define WALLPAPER_PATH "/path/to/your/wallpaper.png"
```

## Controls

- `Ctrl+C` - Exit the application

## Memory Optimization

Zpaper is designed for minimal memory usage:

1. Uses a single SHM buffer per output (no double buffering for static wallpapers)
2. Frees original image data after rendering to SHM buffers
3. Unmaps SHM buffers from user space after commit to Wayland compositor
4. Proper cleanup of all resources on exit

## License

See LICENSE file for details.