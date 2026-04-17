// Linux uinput-based virtual keyboard injector used by scrkbd.
//
// Opens /dev/uinput (requires appropriate permissions, e.g. being a member of
// the `input` group, or a udev rule granting access) and creates a virtual
// keyboard device that can inject key press/release events into the kernel
// input subsystem.  If the device cannot be opened, the class operates in a
// no-op "dry-run" mode so the UI is still usable for debugging.

#pragma once

#include <string>

class VirtualKeyboard
{
public:
    VirtualKeyboard() = default;
    ~VirtualKeyboard();

    // Attempt to open /dev/uinput and register a virtual keyboard.
    // Returns true on success.  When it returns false, last_error() contains
    // a human-readable description of what went wrong; press()/release() will
    // silently become no-ops.
    bool open();

    // Release the uinput device.
    void close();

    bool is_open() const { return fd_ >= 0; }
    const std::string& last_error() const { return last_error_; }

    // Inject a key press or release event.  `keycode` is a Linux KEY_* code
    // from <linux/input-event-codes.h>.
    void press(int keycode);
    void release(int keycode);

    // Convenience: press then release.
    void tap(int keycode);

private:
    void emit(int type, int code, int val);
    void sync();

    int fd_ = -1;
    std::string last_error_;
};
