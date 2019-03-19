#include "tools/DisplayOptions.h"
#include <random>
#include "ui/MainApplication.h"

namespace pepr3d {

void DisplayOptions::drawToSidePane(SidePane& sidePane) {
    ModelView& modelView = mApplication.getModelView();

    bool isFovZoomEnabled = modelView.isFovZoomEnabled();
    sidePane.drawText("Camera zoom behavior:");
    if(ImGui::RadioButton("Dolly (move closer / further)", !isFovZoomEnabled)) {
        modelView.setFovZoomEnabled(false);
    }
    sidePane.drawTooltipOnHover("Scrolling moves the camera closer to the object, potentially entering its inside.");
    if(ImGui::RadioButton("Change field of view", isFovZoomEnabled)) {
        modelView.setFovZoomEnabled(true);
    }
    sidePane.drawTooltipOnHover(
        "Changes the FOV without moving the camera. Can zoom closer than 'dolly', but the camera will get more "
        "sensitive to moving.");
    if(sidePane.drawButton("Reset camera")) {
        modelView.resetCamera();
    }
    sidePane.drawTooltipOnHover("Reset the camera to the default position.");
    sidePane.drawSeparator();

    int modelRoll = static_cast<int>(modelView.getModelRoll());
    if(sidePane.drawIntDragger("Model roll", modelRoll, 0.25f, -180, 180, "%.0f°", 50.0f)) {
        modelView.setModelRoll(static_cast<float>(modelRoll));
    }
    sidePane.drawTooltipOnHover("Rotate the model along the frontal axis.");

    glm::vec3 modelTranslate = modelView.getModelTranslate();
    if(sidePane.drawVec3Dragger("Model position", modelTranslate, 0.01f, -1.0f, 1.0f, "%.2f", 120.0f)) {
        modelView.setModelTranslate(modelTranslate);
    }
    sidePane.drawTooltipOnHover("Move the model along the front-back/up-down/left-right axis.");

    if(sidePane.drawButton("Reset model transformation")) {
        modelView.setModelRoll(0.0f);
        modelView.setModelTranslate(glm::vec3(0.0f));
    }
    sidePane.drawTooltipOnHover("Reset model transformation to the default state.");

    sidePane.drawSeparator();
    sidePane.drawCheckbox("Show grid", modelView.isGridEnabled(),
                          [&](bool isChecked) { modelView.enableGrid(isChecked); });
    sidePane.drawTooltipOnHover(
        "When enabled, a grid will be displayed on the bottom of the model, simulating the printing bed.");
    sidePane.drawCheckbox("Show wireframe", modelView.isWireframeEnabled(),
                          [&](bool isChecked) { modelView.enableWireframe(isChecked); });
    sidePane.drawTooltipOnHover(
        "When enabled, all triangles on the model will have their edges highlighted. Allows for checking the geometry "
        "in Pepr3D.");

    sidePane.drawSeparator();
}

}  // namespace pepr3d