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
#include "UiStateStore.h"
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

   private:
    UiStateStore mState;
};

void MainApplication::setup() {
    setWindowSize(950, 570);
    mState.mainWindow.size = glm::vec2(getWindowSize());
    auto uiOptions = ImGui::Options();
    std::vector<ImWchar> textRange = {0x0001, 0x00BF, 0};
    std::vector<ImWchar> iconsRange = {ICON_MIN_MD, ICON_MAX_MD, 0};
    uiOptions.fonts({make_pair<fs::path, float>(getAssetPath("fonts/SourceSansPro-SemiBold.ttf"), 1.0f * 18.0f),
                     make_pair<fs::path, float>(getAssetPath("fonts/MaterialIcons-Regular.ttf"), 1.0f * 24.0f)},
                    true);
    uiOptions.fontGlyphRanges("SourceSansPro-SemiBold", textRange);
    uiOptions.fontGlyphRanges("MaterialIcons-Regular", iconsRange);
    ImGui::initialize(uiOptions);
    applyLightTheme(ImGui::GetStyle());
    setupModelView(mState);
    mState.modelView.cameraUi = ci::CameraUi(&mState.modelView.camera, getWindow());
    mState.modelView.cameraUi.setWindowSize(mState.modelView.viewport.second);
}

void MainApplication::resize() {
    mState.mainWindow.size = glm::vec2(getWindowSize());
}

void MainApplication::mouseDown( MouseEvent event )
{
    // onModelViewMouseDown(mState, event);
}

void MainApplication::mouseDrag( MouseEvent event )
{
	// onModelViewMouseDrag(mState, event);
}

void MainApplication::update() {}

void MainApplication::draw() {
    gl::clear(ColorA::hex(0xFCFCFC));

    if(mState.mainWindow.showDemoWindow) {
        ImGui::ShowDemoWindow();
    }

    drawToolbar(mState);
    drawSidePane(mState);
    drawModelView(mState);

    ImGui::Begin("##sidepane-debug");
    static int addedValue = 1;
    ImGui::Text("Current value: %i", mState.integerState.mInnerValue);
    if(mState.integerManager.canUndo()) {
        if(ImGui::Button("Undo")) {
            mState.integerManager.undo();
        }
    }
    if(mState.integerManager.canRedo()) {
        if(ImGui::Button("Redo")) {
            mState.integerManager.redo();
        }
    }
    ImGui::SliderInt("##addedvalue", &addedValue, 1, 10);
    ImGui::SameLine();
    if(ImGui::Button("Add")) {
        mState.integerManager.execute(make_unique<AddValueCommand>(addedValue));
    }
    ImGui::End();
}

}  // namespace pepr3d