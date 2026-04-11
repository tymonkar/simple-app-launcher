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

## Customization

simple-app-launcher can be styled with a CSS file placed at:

```
~/.config/sal/style.css
```

The following widgets are available for styling:

| Selector                  | Description                                                                 |
|---------------------------|-----------------------------------------------------------------------------|
| `#main-window`            | The top-level window                                                        |
| `#box`                    | Main container — use `min-width` and `min-height` to control window size    |
| `#input`                  | Search entry                                                                |
| `#input:focus`            | Search entry when focused                                                   |
| `#scroll`                 | Scrolled window wrapping the list                                           |
| `#app-list`               | The application list                                                        |
| `#app-list row`           | Individual application entries                                              |
| `#app-list row:hover`     | App entry on mouse hover                                                    |
| `#app-list row:selected`  | Currently selected entry                                                    |

### Example

```css
#main-window {
    font-size: 14px;
    border-radius: 3px;
    background-color: rgba(20, 20, 21, 1);
    border: 2px solid #8ba9c1;
    color: #ffffff;
}

#box {
    padding: 20px;
    min-width: 400px;
    min-height: 300px;
}

#input {
    color: #ffffff;
    background-color: rgba(20, 20, 21, 1);
    caret-color: #8ba9c1;
    border: none;
    border-bottom: 1px solid #8ba9c1;
    border-radius: 0;
    padding: 4px 2px;
    margin-bottom: 4px;
}

#input:focus {
    border-bottom-color: #ffffff;
}

#scroll {
    padding-top: 2px;
}

#app-list {
    background-color: rgba(20, 20, 21, 1);
}

#app-list row {
    color: #ffffff;
    background-color: rgba(20, 20, 21, 1);
    padding: 4px 2px;
    border-radius: 2px;
}

#app-list row:hover {
    background-color: rgba(255, 255, 255, 0.05);
}

#app-list row:selected {
    background-color: #606079;
    color: #ffffff;
}
```

> **Note:** GTK CSS does not support `width` or `height` on the top-level window. Use `min-width` and `min-height` on `#box` to control the launcher size instead.
