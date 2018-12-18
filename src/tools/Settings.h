#pragma once
#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/SidePane.h"

namespace pepr3d {

class Settings : public Tool {
   public:
    virtual std::string getName() const override {
        return "Settings";
    }

    virtual std::optional<Hotkey> getHotkey(const Hotkeys& hotkeys) const override {
        return hotkeys.findHotkey(HotkeyAction::SelectSettings);
    }

    virtual std::string getIcon() const override {
        return ICON_MD_SETTINGS;
    }
};
}  // namespace pepr3d
