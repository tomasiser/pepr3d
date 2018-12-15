#pragma once

#include <cctype>
#include <optional>
#include <unordered_map>

#include "cinder/app/KeyEvent.h"

namespace pepr3d {

enum class HotkeyAction : std::size_t {
    Open,
    Save,
    Import,
    Export,
    Undo,
    Redo,
    SelectTrianglePainter,
    SelectPaintBucket,
    SelectBrush,
    SelectTextEditor,
    SelectSegmentation,
    SelectDisplayOptions,
    SelectSettings,
    SelectInformation,
    SelectLiveDebug,
    SelectColor1,  // keep colors in order!
    SelectColor2,
    SelectColor3,
    SelectColor4,
    SelectColor5,
    SelectColor6,
    SelectColor7,
    SelectColor8,
    SelectColor9,
    SelectColor10
};

using KeyCode = int;

struct Hotkey {
    KeyCode keycode;
    bool withCtrl;

    bool operator==(const Hotkey& rhs) const {
        return keycode == rhs.keycode && withCtrl == rhs.withCtrl;
    }

    std::string getString() const {
        std::string result;
        if(withCtrl) {
            result += "Ctrl+";
        }
        if(keycode >= ci::app::KeyEvent::KEY_a && keycode <= ci::app::KeyEvent::KEY_z) {
            result += std::toupper('a' + static_cast<char>(keycode - ci::app::KeyEvent::KEY_a));
        } else if(keycode >= ci::app::KeyEvent::KEY_0 && keycode <= ci::app::KeyEvent::KEY_9) {
            result += '0' + static_cast<char>(keycode - ci::app::KeyEvent::KEY_0);
        }
        return result;
    }
};

struct HotkeyHasher {
    std::size_t operator()(const Hotkey& hotkey) const {
        return std::hash<KeyCode>()(hotkey.keycode) ^ std::hash<bool>()(hotkey.withCtrl);
    }
};

class Hotkeys {
    std::unordered_map<Hotkey, HotkeyAction, HotkeyHasher> mKeysToActions;
    std::unordered_map<HotkeyAction, Hotkey> mActionsToKeys;

   public:
    std::optional<Hotkey> findHotkey(HotkeyAction action) const {
        auto found = mActionsToKeys.find(action);
        if(found == mActionsToKeys.end()) {
            return {};
        } else {
            return found->second;
        }
    }

    std::optional<HotkeyAction> findAction(Hotkey hotkey) const {
        auto found = mKeysToActions.find(hotkey);
        if(found == mKeysToActions.end()) {
            return {};
        } else {
            return found->second;
        }
    }

    void add(Hotkey hotkey, HotkeyAction action) {
        mKeysToActions.insert({hotkey, action});
        mActionsToKeys.insert({action, hotkey});
    }

    void loadDefaults() {
        mKeysToActions.clear();
        mActionsToKeys.clear();
        add({ci::app::KeyEvent::KEY_o, true}, HotkeyAction::Open);
        add({ci::app::KeyEvent::KEY_s, true}, HotkeyAction::Save);
        add({ci::app::KeyEvent::KEY_i, true}, HotkeyAction::Import);
        add({ci::app::KeyEvent::KEY_e, true}, HotkeyAction::Export);
        add({ci::app::KeyEvent::KEY_z, true}, HotkeyAction::Undo);
        add({ci::app::KeyEvent::KEY_y, true}, HotkeyAction::Redo);
        add({ci::app::KeyEvent::KEY_t, false}, HotkeyAction::SelectTrianglePainter);
        add({ci::app::KeyEvent::KEY_p, false}, HotkeyAction::SelectPaintBucket);
        add({ci::app::KeyEvent::KEY_b, false}, HotkeyAction::SelectBrush);
        add({ci::app::KeyEvent::KEY_e, false}, HotkeyAction::SelectTextEditor);
        add({ci::app::KeyEvent::KEY_s, false}, HotkeyAction::SelectSegmentation);
        add({ci::app::KeyEvent::KEY_d, false}, HotkeyAction::SelectDisplayOptions);
        add({ci::app::KeyEvent::KEY_g, false}, HotkeyAction::SelectSettings);
        add({ci::app::KeyEvent::KEY_i, false}, HotkeyAction::SelectInformation);
        add({ci::app::KeyEvent::KEY_l, false}, HotkeyAction::SelectLiveDebug);
        add({ci::app::KeyEvent::KEY_1, false}, HotkeyAction::SelectColor1);
        add({ci::app::KeyEvent::KEY_2, false}, HotkeyAction::SelectColor2);
        add({ci::app::KeyEvent::KEY_3, false}, HotkeyAction::SelectColor3);
        add({ci::app::KeyEvent::KEY_4, false}, HotkeyAction::SelectColor4);
        add({ci::app::KeyEvent::KEY_5, false}, HotkeyAction::SelectColor5);
        add({ci::app::KeyEvent::KEY_6, false}, HotkeyAction::SelectColor6);
        add({ci::app::KeyEvent::KEY_7, false}, HotkeyAction::SelectColor7);
        add({ci::app::KeyEvent::KEY_8, false}, HotkeyAction::SelectColor8);
        add({ci::app::KeyEvent::KEY_9, false}, HotkeyAction::SelectColor9);
        add({ci::app::KeyEvent::KEY_0, false}, HotkeyAction::SelectColor10);
    }
};

}  // namespace pepr3d