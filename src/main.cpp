// scrkbd: an on-screen touch keyboard for Linux.
//
// Renders a keyboard with Dear ImGui/GLFW/OpenGL-ES2 and synthesises key
// events via /dev/uinput.  Modifier keys (Shift/Ctrl/Alt/Super) are
// "one-shot": they latch until the next non-modifier key is pressed, then
// release automatically.  A second tap on Shift turns into caps-lock.

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

#include <linux/input-event-codes.h>

#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "keyboard_layout.h"
#include "virtual_keyboard.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// Version strings populated by cmake/version.cmake.
extern "C" const char* GIT_REV;
extern "C" const char* GIT_TAG;
extern "C" const char* GIT_BRANCH;

namespace {

struct Modifiers
{
    // One-shot state: true if the modifier is armed and should be applied to
    // the next key press, then released.
    bool shift_oneshot = false;
    bool ctrl_oneshot  = false;
    bool alt_oneshot   = false;
    bool super_oneshot = false;

    // Latched state: true if the modifier is locked on (e.g. caps-lock).
    bool shift_lock = false;

    bool effective_shift() const { return shift_oneshot || shift_lock; }
};

void glfw_error_callback(int error, const char* description)
{
    std::fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Toggle a shift-like modifier with three-state behaviour:
//   off   -> one-shot
//   one-shot -> locked
//   locked -> off
void cycle_shift(Modifiers& m)
{
    if (m.shift_lock) {
        m.shift_lock = false;
        m.shift_oneshot = false;
    } else if (m.shift_oneshot) {
        m.shift_oneshot = false;
        m.shift_lock = true;
    } else {
        m.shift_oneshot = true;
    }
}

// Press the currently-active modifiers, tap `keycode`, release modifiers,
// then clear any one-shot latches so the next press is unmodified.
void send_key(VirtualKeyboard& kbd, Modifiers& m, int keycode)
{
    if (keycode <= 0)
        return;

    const bool shift = m.effective_shift();
    const bool ctrl  = m.ctrl_oneshot;
    const bool alt   = m.alt_oneshot;
    const bool super = m.super_oneshot;

    if (shift) kbd.press(KEY_LEFTSHIFT);
    if (ctrl)  kbd.press(KEY_LEFTCTRL);
    if (alt)   kbd.press(KEY_LEFTALT);
    if (super) kbd.press(KEY_LEFTMETA);

    kbd.tap(keycode);

    if (super) kbd.release(KEY_LEFTMETA);
    if (alt)   kbd.release(KEY_LEFTALT);
    if (ctrl)  kbd.release(KEY_LEFTCTRL);
    if (shift) kbd.release(KEY_LEFTSHIFT);

    // clear non-locking one-shots
    m.shift_oneshot = false;
    m.ctrl_oneshot  = false;
    m.alt_oneshot   = false;
    m.super_oneshot = false;
}

} // namespace

int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

#if defined(IMGUI_IMPL_OPENGL_ES2)
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
#elif defined(__APPLE__)
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;

    GLFWwindow* window = glfwCreateWindow(1280, 360, "scrkbd", nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        return 1;
    }

    int screen_w = 1280;
    int screen_h = 800;
    if (GLFWmonitor* monitor = glfwGetPrimaryMonitor()) {
        if (const GLFWvidmode* mode = glfwGetVideoMode(monitor)) {
            screen_w = mode->width;
            screen_h = mode->height;
        }
    }
    // Dock the keyboard to the bottom of the primary monitor, full width.
    const int win_w = screen_w;
    const int win_h = screen_h / 3;
    glfwSetWindowMonitor(window, nullptr, 0, screen_h - win_h, win_w, win_h, GLFW_DONT_CARE);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load a larger font so the keys are legible on a touch screen.
    {
        ImFontConfig cfg;
        cfg.SizePixels = 28.0f;
        const char* candidates[] = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            nullptr,
        };
        ImFont* loaded = nullptr;
        for (const char** p = candidates; *p && !loaded; ++p) {
            loaded = io.Fonts->AddFontFromFileTTF(*p, 28.0f);
        }
        if (!loaded) {
            io.Fonts->AddFontDefault(&cfg);
        }
    }

    // Paint the window black for immediate visual feedback while the uinput
    // device and fonts are still being initialised.
    int display_w = 0, display_h = 0;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(window);

    io.IniFilename = "scrkbd.ini";

    VirtualKeyboard kbd;
    const bool kbd_ok = kbd.open();
    if (!kbd_ok) {
        std::fprintf(stderr,
                     "scrkbd: could not initialise uinput (%s).\n"
                     "        UI will run in read-only mode; no key events will be injected.\n"
                     "        Add your user to the `input` group or create a udev rule to grant\n"
                     "        access to /dev/uinput.\n",
                     kbd.last_error().c_str());
    }

    const std::vector<Layout>& layouts = BuiltinLayouts();
    size_t layer_index = 0;
    Modifiers mods;

    while (!glfwWindowShouldClose(window)) {
        // Poll at ~60fps; burn-down sleep loop to keep the app responsive
        // without hogging the CPU.
        static std::chrono::high_resolution_clock::time_point prev;
        glfwPollEvents();
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::microseconds>(now - prev).count() < 16667) {
            std::this_thread::sleep_for(std::chrono::microseconds(500));
            continue;
        }
        prev = now;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
#ifndef DEBUG
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
#endif
        io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);

        glfwGetFramebufferSize(window, &display_w, &display_h);

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)display_w, (float)display_h));
        ImGui::Begin("Keyboard", nullptr,
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);

        const Layout& layer = layouts[layer_index];

        // Compute the widest row so every row aligns to the same grid.
        float max_row_units = 0.0f;
        for (const Row& row : layer.rows) {
            float row_units = 0.0f;
            for (const Key& k : row.keys) row_units += k.width;
            if (row_units > max_row_units) max_row_units = row_units;
        }
        if (max_row_units <= 0.0f) max_row_units = 1.0f;

        const ImGuiStyle& style = ImGui::GetStyle();
        const float avail_w = ImGui::GetContentRegionAvail().x;
        const float avail_h = ImGui::GetContentRegionAvail().y;
        const float spacing_x = style.ItemSpacing.x;
        const float spacing_y = style.ItemSpacing.y;

        const float cell_w = (avail_w - spacing_x * (max_row_units - 1.0f)) / max_row_units;
        const float n_rows = (float)layer.rows.size();
        const float cell_h = (avail_h - spacing_y * (n_rows - 1.0f)) / (n_rows > 0 ? n_rows : 1.0f);

        for (size_t ri = 0; ri < layer.rows.size(); ++ri) {
            const Row& row = layer.rows[ri];
            for (size_t ki = 0; ki < row.keys.size(); ++ki) {
                const Key& k = row.keys[ki];
                if (ki > 0) ImGui::SameLine();

                const float w = cell_w * k.width + spacing_x * (k.width - 1.0f);
                const std::string& label =
                    mods.effective_shift() ? k.shifted_label : k.label;

                // Highlight latched/armed modifier keys.
                bool highlighted = false;
                if (k.action == KeyAction::Shift && (mods.shift_oneshot || mods.shift_lock)) highlighted = true;
                if (k.action == KeyAction::Ctrl  && mods.ctrl_oneshot)  highlighted = true;
                if (k.action == KeyAction::Alt   && mods.alt_oneshot)   highlighted = true;
                if (k.action == KeyAction::Super && mods.super_oneshot) highlighted = true;

                if (highlighted) {
                    ImVec4 c = mods.shift_lock && k.action == KeyAction::Shift
                                    ? ImVec4(0.85f, 0.35f, 0.10f, 1.0f)
                                    : ImVec4(0.20f, 0.55f, 0.85f, 1.0f);
                    ImGui::PushStyleColor(ImGuiCol_Button, c);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, c);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                                          ImVec4(c.x * 1.1f, c.y * 1.1f, c.z * 1.1f, 1.0f));
                }

                // Use row+col in the id so duplicate labels (e.g. two Shift
                // keys) don't collide.
                ImGui::PushID((int)(ri * 64 + ki));
                const bool pressed = ImGui::Button(label.c_str(), ImVec2(w, cell_h));
                ImGui::PopID();

                if (highlighted)
                    ImGui::PopStyleColor(3);

                if (!pressed)
                    continue;

                switch (k.action) {
                case KeyAction::Shift:
                    cycle_shift(mods);
                    break;
                case KeyAction::Ctrl:
                    mods.ctrl_oneshot = !mods.ctrl_oneshot;
                    break;
                case KeyAction::Alt:
                    mods.alt_oneshot = !mods.alt_oneshot;
                    break;
                case KeyAction::Super:
                    mods.super_oneshot = !mods.super_oneshot;
                    break;
                case KeyAction::SwitchLayer:
                    layer_index = (layer_index + 1) % layouts.size();
                    break;
                case KeyAction::Close:
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                    break;
                case KeyAction::None:
                    send_key(kbd, mods, k.keycode);
                    break;
                }
            }
        }

        ImGui::End();

        // Rendering
        ImGui::Render();
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        // Touch-screen workaround: move the cursor off-screen after release
        // so a stray "hover" doesn't linger on the last key the finger
        // lifted from.  Skipped in debug builds where a real mouse is used.
#ifndef DEBUG
        {
            static bool prev_frame_mouse_down = false;
            if (prev_frame_mouse_down && !io.MouseDown[0]) {
                io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
            }
            prev_frame_mouse_down = io.MouseDown[0];
        }
#endif

        glfwSwapBuffers(window);
    }

    (void)GIT_REV; (void)GIT_TAG; (void)GIT_BRANCH;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
