#include "virtual_keyboard.h"

#include <cerrno>
#include <cstring>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <linux/uinput.h>

VirtualKeyboard::~VirtualKeyboard()
{
    close();
}

bool VirtualKeyboard::open()
{
    if (fd_ >= 0)
        return true;

    fd_ = ::open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd_ < 0) {
        last_error_ = std::string("open(/dev/uinput): ") + std::strerror(errno);
        return false;
    }

    if (ioctl(fd_, UI_SET_EVBIT, EV_KEY) < 0 ||
        ioctl(fd_, UI_SET_EVBIT, EV_SYN) < 0) {
        last_error_ = std::string("UI_SET_EVBIT: ") + std::strerror(errno);
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    // Enable every KEY_* code in the common range so we can inject any of
    // them without needing to enumerate the layout up front.  KEY_MAX is
    // defined by the kernel headers; we stop a bit below it to avoid codes
    // that aren't actually keys.
    for (int code = 1; code < 256; ++code) {
        if (ioctl(fd_, UI_SET_KEYBIT, code) < 0) {
            last_error_ = std::string("UI_SET_KEYBIT: ") + std::strerror(errno);
            ::close(fd_);
            fd_ = -1;
            return false;
        }
    }

    struct uinput_setup usetup;
    std::memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor  = 0x1234;
    usetup.id.product = 0x5678;
    usetup.id.version = 1;
    std::snprintf(usetup.name, sizeof(usetup.name), "%s", "scrkbd virtual keyboard");

    if (ioctl(fd_, UI_DEV_SETUP, &usetup) < 0) {
        last_error_ = std::string("UI_DEV_SETUP: ") + std::strerror(errno);
        ::close(fd_);
        fd_ = -1;
        return false;
    }
    if (ioctl(fd_, UI_DEV_CREATE) < 0) {
        last_error_ = std::string("UI_DEV_CREATE: ") + std::strerror(errno);
        ::close(fd_);
        fd_ = -1;
        return false;
    }
    last_error_.clear();
    return true;
}

void VirtualKeyboard::close()
{
    if (fd_ < 0)
        return;
    ioctl(fd_, UI_DEV_DESTROY);
    ::close(fd_);
    fd_ = -1;
}

void VirtualKeyboard::emit(int type, int code, int val)
{
    if (fd_ < 0)
        return;
    struct input_event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.type = type;
    ev.code = code;
    ev.value = val;
    // timestamp is filled in by the kernel when it's zero.
    ssize_t n = ::write(fd_, &ev, sizeof(ev));
    (void)n;
}

void VirtualKeyboard::sync()
{
    emit(EV_SYN, SYN_REPORT, 0);
}

void VirtualKeyboard::press(int keycode)
{
    if (keycode <= 0) return;
    emit(EV_KEY, keycode, 1);
    sync();
}

void VirtualKeyboard::release(int keycode)
{
    if (keycode <= 0) return;
    emit(EV_KEY, keycode, 0);
    sync();
}

void VirtualKeyboard::tap(int keycode)
{
    if (keycode <= 0) return;
    emit(EV_KEY, keycode, 1);
    sync();
    emit(EV_KEY, keycode, 0);
    sync();
}
