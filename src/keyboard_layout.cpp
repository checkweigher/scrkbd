#include "keyboard_layout.h"

#include <linux/input-event-codes.h>

namespace {

Key K(const std::string& label, const std::string& shifted, int code, float width = 1.0f)
{
    Key k;
    k.label = label;
    k.shifted_label = shifted.empty() ? label : shifted;
    k.keycode = code;
    k.width = width;
    return k;
}

Key A(const std::string& label, KeyAction action, int code = 0, float width = 1.0f)
{
    Key k;
    k.label = label;
    k.shifted_label = label;
    k.keycode = code;
    k.width = width;
    k.action = action;
    return k;
}

Layout make_alpha()
{
    Layout l;
    l.name = "alpha";
    l.rows = {
        { {
            K("`", "~", KEY_GRAVE),
            K("1", "!", KEY_1), K("2", "@", KEY_2), K("3", "#", KEY_3),
            K("4", "$", KEY_4), K("5", "%", KEY_5), K("6", "^", KEY_6),
            K("7", "&", KEY_7), K("8", "*", KEY_8), K("9", "(", KEY_9),
            K("0", ")", KEY_0),
            K("-", "_", KEY_MINUS), K("=", "+", KEY_EQUAL),
            A("Back", KeyAction::None, KEY_BACKSPACE, 2.0f),
        } },
        { {
            A("Tab", KeyAction::None, KEY_TAB, 1.5f),
            K("q", "Q", KEY_Q), K("w", "W", KEY_W), K("e", "E", KEY_E),
            K("r", "R", KEY_R), K("t", "T", KEY_T), K("y", "Y", KEY_Y),
            K("u", "U", KEY_U), K("i", "I", KEY_I), K("o", "O", KEY_O),
            K("p", "P", KEY_P),
            K("[", "{", KEY_LEFTBRACE), K("]", "}", KEY_RIGHTBRACE),
            K("\\", "|", KEY_BACKSLASH, 1.5f),
        } },
        { {
            A("Caps", KeyAction::Shift, 0, 1.75f),
            K("a", "A", KEY_A), K("s", "S", KEY_S), K("d", "D", KEY_D),
            K("f", "F", KEY_F), K("g", "G", KEY_G), K("h", "H", KEY_H),
            K("j", "J", KEY_J), K("k", "K", KEY_K), K("l", "L", KEY_L),
            K(";", ":", KEY_SEMICOLON), K("'", "\"", KEY_APOSTROPHE),
            A("Enter", KeyAction::None, KEY_ENTER, 2.25f),
        } },
        { {
            A("Shift", KeyAction::Shift, 0, 2.25f),
            K("z", "Z", KEY_Z), K("x", "X", KEY_X), K("c", "C", KEY_C),
            K("v", "V", KEY_V), K("b", "B", KEY_B), K("n", "N", KEY_N),
            K("m", "M", KEY_M),
            K(",", "<", KEY_COMMA), K(".", ">", KEY_DOT), K("/", "?", KEY_SLASH),
            A("Shift", KeyAction::Shift, 0, 2.75f),
        } },
        { {
            A("Ctrl", KeyAction::Ctrl, 0, 1.25f),
            A("Super", KeyAction::Super, 0, 1.25f),
            A("Alt", KeyAction::Alt, 0, 1.25f),
            K(" ", " ", KEY_SPACE, 6.5f),
            A("Alt", KeyAction::Alt, 0, 1.25f),
            A("#+=", KeyAction::SwitchLayer, 0, 1.25f),
            A("Esc", KeyAction::None, KEY_ESC, 1.25f),
            A("X", KeyAction::Close, 0, 1.0f),
        } },
    };
    return l;
}

Layout make_symbols()
{
    Layout l;
    l.name = "symbols";
    l.rows = {
        { {
            K("F1", "F1", KEY_F1), K("F2", "F2", KEY_F2), K("F3", "F3", KEY_F3),
            K("F4", "F4", KEY_F4), K("F5", "F5", KEY_F5), K("F6", "F6", KEY_F6),
            K("F7", "F7", KEY_F7), K("F8", "F8", KEY_F8), K("F9", "F9", KEY_F9),
            K("F10", "F10", KEY_F10), K("F11", "F11", KEY_F11), K("F12", "F12", KEY_F12),
            A("Del", KeyAction::None, KEY_DELETE, 2.0f),
        } },
        { {
            K("7", "7", KEY_7), K("8", "8", KEY_8), K("9", "9", KEY_9),
            K("/", "/", KEY_SLASH),
            K("!", "!", KEY_1), K("@", "@", KEY_2), K("#", "#", KEY_3),
            K("$", "$", KEY_4), K("%", "%", KEY_5), K("^", "^", KEY_6),
            K("&", "&", KEY_7), K("*", "*", KEY_8),
            A("Back", KeyAction::None, KEY_BACKSPACE, 2.0f),
        } },
        { {
            K("4", "4", KEY_4), K("5", "5", KEY_5), K("6", "6", KEY_6),
            K("*", "*", KEY_8),
            K("(", "(", KEY_9), K(")", ")", KEY_0),
            K("[", "[", KEY_LEFTBRACE), K("]", "]", KEY_RIGHTBRACE),
            K("{", "{", KEY_LEFTBRACE), K("}", "}", KEY_RIGHTBRACE),
            K("<", "<", KEY_COMMA), K(">", ">", KEY_DOT),
            A("Enter", KeyAction::None, KEY_ENTER, 2.0f),
        } },
        { {
            K("1", "1", KEY_1), K("2", "2", KEY_2), K("3", "3", KEY_3),
            K("-", "-", KEY_MINUS),
            K("_", "_", KEY_MINUS), K("=", "=", KEY_EQUAL), K("+", "+", KEY_EQUAL),
            K("|", "|", KEY_BACKSLASH), K("\\", "\\", KEY_BACKSLASH),
            K("~", "~", KEY_GRAVE), K("`", "`", KEY_GRAVE),
            A("Up", KeyAction::None, KEY_UP),
            A("PgUp", KeyAction::None, KEY_PAGEUP),
        } },
        { {
            K("0", "0", KEY_0), K(".", ".", KEY_DOT),
            A("Home", KeyAction::None, KEY_HOME),
            A("End", KeyAction::None, KEY_END),
            A("Ins", KeyAction::None, KEY_INSERT),
            A("ESC", KeyAction::None, KEY_ESC),
            K(" ", " ", KEY_SPACE, 2.0f),
            A("abc", KeyAction::SwitchLayer, 0, 1.25f),
            A("Left", KeyAction::None, KEY_LEFT),
            A("Down", KeyAction::None, KEY_DOWN),
            A("Right", KeyAction::None, KEY_RIGHT),
            A("PgDn", KeyAction::None, KEY_PAGEDOWN),
        } },
    };
    return l;
}

} // namespace

const std::vector<Layout>& BuiltinLayouts()
{
    static const std::vector<Layout> layouts = { make_alpha(), make_symbols() };
    return layouts;
}
