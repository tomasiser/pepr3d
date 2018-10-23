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

#include "tools/Tool.h"
#include "tools/TrianglePainter.h"
#include "tools/Brush.h"
#include "tools/PaintBucket.h"
#include "tools/TextEditor.h"
#include "tools/Segmentation.h"
#include "tools/DisplayOptions.h"
#include "tools/Settings.h"
#include "tools/Information.h"
#include "tools/LiveDebug.h"

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

    Toolbar& getToolbar() {
        return mToolbar;
    }

    SidePane& getSidePane() {
        return mSidePane;
    }

    ModelView& getModelView() {
        return mModelView;
    }

    bool isDemoWindowShown() {
        return mShowDemoWindow;
    }

    void showDemoWindow(bool show = true) {
        mShowDemoWindow = show;
    }

    using ToolsVector = std::vector<std::unique_ptr<ITool>>;

    ToolsVector::iterator getToolsBegin() {
        return mTools.begin();
    }

    ToolsVector::iterator getToolsEnd() {
        return mTools.end();
    }

    ToolsVector::iterator getCurrentToolIterator() {
        assert(mTools.size() > 0);
        assert(mCurrentToolIterator != mTools.end());
        return mCurrentToolIterator;
    }

    void setCurrentToolIterator(ToolsVector::iterator tool) {
        assert(mTools.size() > 0);
        assert(tool != mTools.end());
        mCurrentToolIterator = tool;
    }

   private:
    Toolbar mToolbar;
    SidePane mSidePane;
    ModelView mModelView;
    bool mShowDemoWindow = false;

    ToolsVector mTools;
    ToolsVector::iterator mCurrentToolIterator;
};

}  // namespace pepr3d
