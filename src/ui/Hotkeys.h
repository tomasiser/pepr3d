#pragma once

#include <cctype>
#include <optional>
#include <unordered_map>

#include "cinder/app/KeyEvent.h"

#include <cereal/types/unordered_map.hpp>

namespace pepr3d {

/// Enumeration of all actions that can happen via a hotkey
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
    SelectSemiautomaticSegmentation,
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
    SelectColor10,
    InvalidAction
};

template <class Archive>
std::string save_minimal(const Archive&, const HotkeyAction& action) {
    switch(action) {
    case HotkeyAction::Open: return "Open";
    case HotkeyAction::Save: return "Save";
    case HotkeyAction::Import: return "Import";
    case HotkeyAction::Export: return "Export";
    case HotkeyAction::Undo: return "Undo";
    case HotkeyAction::Redo: return "Redo";
    case HotkeyAction::SelectTrianglePainter: return "SelectTrianglePainter";
    case HotkeyAction::SelectPaintBucket: return "SelectPaintBucket";
    case HotkeyAction::SelectBrush: return "SelectBrush";
    case HotkeyAction::SelectTextEditor: return "SelectTextEditor";
    case HotkeyAction::SelectSegmentation: return "SelectSegmentation";
    case HotkeyAction::SelectSemiautomaticSegmentation: return "SelectSemiautomaticSegmentation";
    case HotkeyAction::SelectDisplayOptions: return "SelectDisplayOptions";
    case HotkeyAction::SelectSettings: return "SelectSettings";
    case HotkeyAction::SelectInformation: return "SelectInformation";
    case HotkeyAction::SelectLiveDebug: return "SelectLiveDebug";
    case HotkeyAction::SelectColor1: return "SelectColor1";
    case HotkeyAction::SelectColor2: return "SelectColor2";
    case HotkeyAction::SelectColor3: return "SelectColor3";
    case HotkeyAction::SelectColor4: return "SelectColor4";
    case HotkeyAction::SelectColor5: return "SelectColor5";
    case HotkeyAction::SelectColor6: return "SelectColor6";
    case HotkeyAction::SelectColor7: return "SelectColor7";
    case HotkeyAction::SelectColor8: return "SelectColor8";
    case HotkeyAction::SelectColor9: return "SelectColor9";
    case HotkeyAction::SelectColor10: return "SelectColor10";
    }
    return "InvalidAction";
}

template <class Archive>
void load_minimal(const Archive&, HotkeyAction& action, const std::string& value) {
    if(value == "Open") {
        action = HotkeyAction::Open;
    } else if(value == "Save") {
        action = HotkeyAction::Save;
    } else if(value == "Import") {
        action = HotkeyAction::Import;
    } else if(value == "Export") {
        action = HotkeyAction::Export;
    } else if(value == "Undo") {
        action = HotkeyAction::Undo;
    } else if(value == "Redo") {
        action = HotkeyAction::Redo;
    } else if(value == "SelectTrianglePainter") {
        action = HotkeyAction::SelectTrianglePainter;
    } else if(value == "SelectPaintBucket") {
        action = HotkeyAction::SelectPaintBucket;
    } else if(value == "SelectBrush") {
        action = HotkeyAction::SelectBrush;
    } else if(value == "SelectTextEditor") {
        action = HotkeyAction::SelectTextEditor;
    } else if(value == "SelectSegmentation") {
        action = HotkeyAction::SelectSegmentation;
    } else if(value == "SelectSemiautomaticSegmentation") {
        action = HotkeyAction::SelectSemiautomaticSegmentation;
    } else if(value == "SelectDisplayOptions") {
        action = HotkeyAction::SelectDisplayOptions;
    } else if(value == "SelectSettings") {
        action = HotkeyAction::SelectSettings;
    } else if(value == "SelectInformation") {
        action = HotkeyAction::SelectInformation;
    } else if(value == "SelectLiveDebug") {
        action = HotkeyAction::SelectLiveDebug;
    } else if(value == "SelectColor1") {
        action = HotkeyAction::SelectColor1;
    } else if(value == "SelectColor2") {
        action = HotkeyAction::SelectColor2;
    } else if(value == "SelectColor3") {
        action = HotkeyAction::SelectColor3;
    } else if(value == "SelectColor4") {
        action = HotkeyAction::SelectColor4;
    } else if(value == "SelectColor5") {
        action = HotkeyAction::SelectColor5;
    } else if(value == "SelectColor6") {
        action = HotkeyAction::SelectColor6;
    } else if(value == "SelectColor7") {
        action = HotkeyAction::SelectColor7;
    } else if(value == "SelectColor8") {
        action = HotkeyAction::SelectColor8;
    } else if(value == "SelectColor9") {
        action = HotkeyAction::SelectColor9;
    } else if(value == "SelectColor10") {
        action = HotkeyAction::SelectColor10;
    } else {
        action = HotkeyAction::InvalidAction;
    }
}

using KeyCode = int;

/// A hotkey, i.e., a keycode and modifiers (Ctrl)
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

    template <class Archive>
    void serialize(Archive& archive) {
        archive(cereal::make_nvp("keycode", keycode), cereal::make_nvp("ctrl", withCtrl));
    }
};

/// Used for hashing a hotkey so it can be put in std::unordered_map
struct HotkeyHasher {
    std::size_t operator()(const Hotkey& hotkey) const {
        return std::hash<KeyCode>()(hotkey.keycode) ^ std::hash<bool>()(hotkey.withCtrl);
    }
};

/// All hotkeys and their corresponding actions
class Hotkeys {
    std::unordered_map<Hotkey, HotkeyAction, HotkeyHasher> mKeysToActions;
    std::unordered_map<HotkeyAction, Hotkey> mActionsToKeys;

    friend class cereal::access;

   public:
    /// Returns a hotkey for a certain action
    std::optional<Hotkey> findHotkey(HotkeyAction action) const {
        auto found = mActionsToKeys.find(action);
        if(found == mActionsToKeys.end()) {
            return {};
        } else {
            return found->second;
        }
    }

    /// Returns an action for a certain hotkey
    std::optional<HotkeyAction> findAction(Hotkey hotkey) const {
        auto found = mKeysToActions.find(hotkey);
        if(found == mKeysToActions.end()) {
            return {};
        } else {
            return found->second;
        }
    }

    /// Adds a new hotkey & action pair
    void add(Hotkey hotkey, HotkeyAction action) {
        mKeysToActions.insert({hotkey, action});
        mActionsToKeys.insert({action, hotkey});
    }

    /// Loads default hotkeys
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
        add({ci::app::KeyEvent::KEY_m, false}, HotkeyAction::SelectSemiautomaticSegmentation);
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

   private:
    template <class Archive>
    void save(Archive& saveArchive) const {
        saveArchive(cereal::make_nvp("keysToActions", mKeysToActions));
    }

    template <class Archive>
    void load(Archive& loadArchive) {
        loadArchive(cereal::make_nvp("keysToActions", mKeysToActions));
        for(const auto& keyToAction : mKeysToActions) {
            mActionsToKeys.insert({keyToAction.second, keyToAction.first});
        }
        assert(mActionsToKeys.size() == mKeysToActions.size());
    }
};

}  // namespace pepr3d