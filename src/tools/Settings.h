#pragma once
#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/SidePane.h"

namespace pepr3d {

class Settings : public Tool {
    MainApplication& mApplication;
    SidePane::Category mColorPaletteCategory;
    SidePane::Category mUiCategory;

   public:
    Settings(MainApplication& app)
        : mApplication(app), mColorPaletteCategory("Edit Color Palette", true), mUiCategory("User Interface", true) {}

    virtual std::string getName() const override {
        return "Settings";
    }

    virtual std::string getDescription() const override {
        return "Configure the color palette and application.";
    }

    virtual std::optional<Hotkey> getHotkey(const Hotkeys& hotkeys) const override {
        return hotkeys.findHotkey(HotkeyAction::SelectSettings);
    }

    virtual std::string getIcon() const override {
        return ICON_MD_SETTINGS;
    }

    virtual void drawToSidePane(SidePane& sidePane) override;

    void drawUiSettings(SidePane& sidePane);
};
}  // namespace pepr3d
