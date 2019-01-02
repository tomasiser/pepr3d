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
    if(ImGui::RadioButton("Change field of view", isFovZoomEnabled)) {
        modelView.setFovZoomEnabled(true);
    }
    if(sidePane.drawButton("Reset camera")) {
        modelView.resetCamera();
    }
    sidePane.drawSeparator();

    int modelRoll = static_cast<int>(modelView.getModelRoll());
    sidePane.drawIntDragger("Model roll", modelRoll, 0.25f, -180, 180, "%.0f°", 50.0f);
    modelView.setModelRoll(static_cast<float>(modelRoll));

    glm::vec3 modelTranslate = modelView.getModelTranslate();
    sidePane.drawVec3Dragger("Model position", modelTranslate, 0.01f, -1.0f, 1.0f, "%.2f", 120.0f);
    modelView.setModelTranslate(modelTranslate);

    if(sidePane.drawButton("Reset model translation")) {
        modelView.setModelRoll(0.0f);
        modelView.setModelTranslate(glm::vec3(0.0f));
    }

    sidePane.drawSeparator();
    sidePane.drawCheckbox("Show grid", modelView.isGridEnabled(),
                          [&](bool isChecked) { modelView.enableGrid(isChecked); });
    sidePane.drawCheckbox("Show wireframe", modelView.isWireframeEnabled(),
                          [&](bool isChecked) { modelView.enableWireframe(isChecked); });
}

}  // namespace pepr3d