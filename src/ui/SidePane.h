#pragma once

#include "CinderImGui.h"

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

   private:
    MainApplication& mApplication;
    float mWidth = 235.0f;
};

}  // namespace pepr3d