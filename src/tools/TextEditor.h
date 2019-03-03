#pragma once
#include <string>
#include <vector>

#include "geometry/FontRasterizer.h"
#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/MainApplication.h"
#include "ui/SidePane.h"

namespace pepr3d {

/// A tool used for painting texts on a model
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
    std::string mText = "Pepr";
    int mFontSize = 12;
    int mBezierSteps = 3;
    float mFontScale = 0.05f;

    std::vector<std::vector<FontRasterizer::Tri>> mRenderedText;

    std::vector<std::vector<FontRasterizer::Tri>> triangulateText() const;
    void renderText(std::vector<std::vector<FontRasterizer::Tri>>& result, bool reset) const;
    void processText();
    void rotateText(std::vector<std::vector<FontRasterizer::Tri>>& result) const;
    void rescaleText(std::vector<std::vector<FontRasterizer::Tri>>& result);
};
}  // namespace pepr3d
