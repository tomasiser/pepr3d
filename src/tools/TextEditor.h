#pragma once
#include <string>

#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/MainApplication.h"
#include "ui/SidePane.h"

namespace pepr3d {

class TextEditor : public Tool {
   public:
    TextEditor(MainApplication& app) : mApplication(app) {
        auto fontPath = getAssetPath("fonts/OpenSans-Regular.ttf");
        mFont = fontPath.filename().string();
        mFontPath = fontPath.string();
    }

    virtual std::string getName() const override {
        return "Text Editor";
    }

    virtual std::optional<Hotkey> getHotkey(const Hotkeys& hotkeys) const override {
        return hotkeys.findHotkey(HotkeyAction::SelectTextEditor);
    }

    virtual std::string getIcon() const override {
        return ICON_MD_TEXT_FIELDS;
    }

    virtual void drawToSidePane(SidePane& sidePane) override;

   private:
    MainApplication& mApplication;

    std::string mFont;
    std::string mFontPath;
    std::string mText;
    int mFontSize = 30;
    int mBezierSteps = 1;

    void triangulateText() const;
};
}  // namespace pepr3d
