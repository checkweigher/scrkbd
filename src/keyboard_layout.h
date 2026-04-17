// Keyboard layout definitions used by the scrkbd UI.
//
// A layout is a collection of rows; each row is a sequence of keys.  Every
// key has a Linux KEY_* keycode, a label for the unshifted state, and a
// label for the shifted state (which may be identical to the unshifted one
// for non-letter keys).  Some keys are "action" keys such as shift, backspace
// or layer-switch – those carry an `action` enum and the injector treats
// them specially.

#pragma once

#include <vector>
#include <string>

enum class KeyAction
{
    None = 0,
    Shift,       // toggle one-shot shift (second tap = caps lock)
    Ctrl,        // one-shot ctrl modifier
    Alt,         // one-shot alt modifier
    Super,       // one-shot super/meta modifier
    SwitchLayer, // cycle to next layer
    Close,       // exit the application
};

struct Key
{
    std::string label;         // unshifted label
    std::string shifted_label; // shifted label (same as label if none)
    int keycode = 0;           // Linux KEY_* code, or 0 for pure action keys
    float width = 1.0f;        // multiples of a single cell
    KeyAction action = KeyAction::None;
};

struct Row
{
    std::vector<Key> keys;
};

struct Layout
{
    std::string name;
    std::vector<Row> rows;
};

// Return the built-in layouts: alpha (QWERTY) and symbols.
const std::vector<Layout>& BuiltinLayouts();
