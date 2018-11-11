#pragma once

#include "geometry/ColorManager.h"
#include "peprimgui.h"

namespace pepr3d {

class MainApplication;

class SidePane {
   public:
    SidePane(MainApplication& app) : mApplication(app) {}

    float getWidth() {
        return mWidth;
    }

    void draw();

    void drawText(std::string text);
    bool drawButton(std::string label);
    void drawSeparator();
    void drawColorPalette(ColorManager& colorManager);

    template <typename Callback>
    void drawCheckbox(std::string label, bool isChecked, Callback onChanged) {
        if(ImGui::Checkbox(label.c_str(), &isChecked)) {
            onChanged(isChecked);
        }
    }

   private:
    MainApplication& mApplication;
    float mWidth = 235.0f;
};

}  // namespace pepr3d