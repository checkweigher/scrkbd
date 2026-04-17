# scrkbd

An on-screen touch keyboard for Linux, built with [Dear ImGui](https://github.com/ocornut/imgui),
GLFW and OpenGL ES 2.  Typed keystrokes are injected into the kernel input
subsystem via `/dev/uinput`, so the virtual keyboard works with any
application – X11, Wayland, or the console.

## Features

- QWERTY alphanumeric layer and a symbols / function-key / navigation layer.
- One-shot `Shift`, `Ctrl`, `Alt` and `Super` modifiers that auto-release
  after the next key press.  Tapping `Shift` twice latches caps-lock.
- Key labels follow the active `Shift`/caps state so you always see what
  will actually be typed.
- Touch-screen friendly: cursor is hidden in release builds and a
  workaround is applied so hover styling doesn't stick to the last key
  the finger lifted from.
- Graceful fallback: if `/dev/uinput` can't be opened the UI still runs,
  making it easy to iterate on the layout without root.

## Build

Dependencies: `cmake` (>= 3.14), a C++17 compiler, `pkg-config`,
`libglfw3-dev`, `libgles2-mesa-dev`.

```bash
mkdir build && cd build
cmake ..
make -j
```

The resulting binary is `build/scrkbd`.

## Running

`scrkbd` writes to `/dev/uinput`, which is normally only accessible to
root.  Either run it as root for testing, or (preferred) give your user
access once via a udev rule:

```
# /etc/udev/rules.d/60-uinput.rules
KERNEL=="uinput", MODE="0660", GROUP="input", OPTIONS+="static_node=uinput"
```

Then add yourself to the `input` group:

```bash
sudo usermod -aG input "$USER"
```

Re-login and run `./build/scrkbd`.

## Layout

- Two layers: the alphanumeric QWERTY layer and a symbols / function-key
  layer.  Tap `#+=` / `abc` to cycle between them.
- Modifier keys:
  - `Shift` / `Caps`: tap once for a one-shot shift, twice to latch caps.
  - `Ctrl`, `Alt`, `Super`: tap to arm a one-shot modifier; it will be
    held down for the next non-modifier key press and then released.
- `X` (bottom-right of the alpha layer) closes the application.

Layouts are defined in [`src/keyboard_layout.cpp`](src/keyboard_layout.cpp);
add new rows or layers there.
