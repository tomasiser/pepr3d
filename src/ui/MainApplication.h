#pragma once

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/TextureFont.h"
#include "cinder/gl/gl.h"

#define CINDER_IMGUI_NO_NAMESPACE_ALIAS
#include "CinderImGui.h"

#include "IconsMaterialDesign.h"
#include "LightTheme.h"
#include "ModelView.h"
#include "SidePane.h"
#include "Toolbar.h"
#include "commands/ExampleCommand.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace pepr3d {

class MainApplication : public App {
   public:
    void setup() override;
    void update() override;
    void draw() override;
    void resize() override;
    void mouseDown(MouseEvent event) override;
    void mouseDrag(MouseEvent event) override;

    MainApplication();

    Toolbar& getToolbar() { return mToolbar; }
    SidePane& getSidePane() { return mSidePane; }
    ModelView& getModelView() { return mModelView; }

    bool isDemoWindowShown() { return mShowDemoWindow; }
    void showDemoWindow(bool show = true) { mShowDemoWindow = show; }

   private:
    Toolbar mToolbar;
    SidePane mSidePane;
    ModelView mModelView;
    bool mShowDemoWindow = false;

    IntegerState mIntegerState;
    CommandManager<IntegerState> mIntegerManager;
};

}  // namespace pepr3d
