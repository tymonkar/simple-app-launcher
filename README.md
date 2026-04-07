# simple-app-launcher

A minimal application launcher for Wayland compositors, built with GTK4 and gtk4-layer-shell. It presents a fuzzy-searchable list of installed desktop applications and launches the selected one.

## Features

- Displays all installed applications that expose themselves via `.desktop` files
- Fuzzy search with scored ranking (prefix matches rank higher)
- Keyboard-driven navigation — no mouse needed
- Renders as a Wayland overlay layer, floating above other windows

## Dependencies

- GTK 4
- [gtk4-layer-shell](https://github.com/wmww/gtk4-layer-shell)
- GLib / GIO (bundled with GTK)
- A Wayland compositor that supports the `wlr-layer-shell` protocol (e.g. Sway, Hyprland, Wayfire)

## Building

```sh
make
```

To clean the build output:

```sh
make clean
```

The Makefile uses `pkg-config` to resolve flags for `gtk4`, `gtk4-layer-shell-0`, and `gio-unix-2.0` automatically.

## Usage

Run the binary directly:

```sh
./launcher
```

| Key        | Action                        |
|------------|-------------------------------|
| Type       | Filter the application list   |
| `↑` / `↓` | Move selection up / down      |
| `Enter`    | Launch selected application   |
| `Escape`   | Close the launcher            |



