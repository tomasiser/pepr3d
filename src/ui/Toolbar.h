#pragma once

#include "CinderImGui.h"
#include "IconsMaterialDesign.h"

namespace pepr3d {

class MainApplication;

class Toolbar {
   public:
    Toolbar(MainApplication& app) : mApplication(app) {}

    float getHeight() {
        return mHeight;
    }

    void draw();

   private:
    MainApplication& mApplication;
    std::size_t mSelectedButtonIndex = 3;
    float mHeight = 50.0f;

    void drawSeparator();
    void drawButton(std::size_t index, const char* text);
    void drawDemoWindowButton();
};

}  // namespace pepr3d