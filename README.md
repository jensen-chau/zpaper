# Zpaper

A lightweight Wayland wallpaper application written in C.

## Features

- Static wallpaper support
- Client-daemon architecture for easy management
- Multi-monitor support with independent wallpaper for each output
- Low memory footprint - releases image data after rendering
- Layer-shell protocol integration
- Cover mode scaling (fills screen while maintaining aspect ratio)
- Config file persistence

## Requirements

- Wayland compositor with layer-shell protocol support
- C compiler (GCC or Clang)
- Wayland development libraries
- pkg-config

## Building

```bash
make
```

This builds two executables:
- `zpaper` - Client for controlling the daemon
- `zpaperd` - Daemon that renders wallpapers

## Usage

### Start the daemon

```bash
./zpaperd &
```

The daemon will load the last saved wallpaper from config. If no config exists, it will start without a wallpaper.

### Set wallpaper

```bash
# Set wallpaper for all outputs
./zpaper set /path/to/wallpaper.png

# Set wallpaper for specific output
./zpaper set /path/to/wallpaper.png eDP-1
```

### Other commands

```bash
./zpaper status        # Check if daemon is running
./zpaper stop          # Stop the daemon
./zpaper get           # Get current wallpaper
./zpaper list          # List outputs and wallpapers
./zpaper reload        # Reload configuration
```

## Configuration

Wallpaper settings are saved to `~/.config/zpaper/config.json`:

```json
{
  "default": "/path/to/default/wallpaper.png",
  "outputs": {
    "HDMI-1": {
      "wallpaper": "/path/to/hdmi-wallpaper.png"
    },
    "eDP-1": {
      "wallpaper": "/path/to/laptop-wallpaper.png"
    }
  }
}
```

## Memory Optimization

Zpaper is designed for minimal memory usage:

1. Uses a single SHM buffer per output (no double buffering for static wallpapers)
2. Frees original image data after rendering to SHM buffers
3. Unmaps SHM buffers from user space after commit to Wayland compositor
4. Proper cleanup of all resources on exit

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        zpaper                                │
│  ┌─────────────────┐         ┌─────────────────────────────┐ │
│  │    zpaper       │         │         zpaperd            │ │
│  │    (client)     │◄───────►│         (daemon)           │ │
│  │                 │ Unix    │                             │ │
│  │  - set <path>   │ Socket  │  - Wayland connection      │ │
│  │  - get          │         │  - Wallpaper rendering      │ │
│  │  - list         │         │  - Config management       │ │
│  │  - reload       │         │  - Output management       │ │
│  └─────────────────┘         └─────────────────────────────┘ │
│                                     │                        │
│                                     ▼                        │
│                            ┌─────────────────┐              │
│                            │   Config file  │              │
│                            │ ~/.config/zpaper│              │
│                            │   /config.json  │              │
│                            └─────────────────┘              │
└─────────────────────────────────────────────────────────────┘
```

## License

See LICENSE file for details.
