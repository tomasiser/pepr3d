#include "tools/ExportAssistant.h"
#include <random>
#include <vector>
#include "commands/CmdPaintSingleColor.h"
#include "geometry/SdfValuesException.h"
#include "ui/MainApplication.h"

namespace pepr3d {

void ExportAssistant::drawToSidePane(SidePane& sidePane) {
    auto& modelView = mApplication.getModelView();

    float minHeightPercent = mPreviewMinMaxHeight.x * 100.0f;
    float maxHeightPercent = mPreviewMinMaxHeight.y * 100.0f;
    sidePane.drawText("Preview height range:");
    if(ImGui::DragFloatRange2("##range", &minHeightPercent, &maxHeightPercent, 0.25f, 0.0f, 100.0f, "Min: %.1f %%",
                              "Max: %.1f %%")) {
        minHeightPercent = std::min(minHeightPercent, 99.9f);
        maxHeightPercent = std::max(0.1f, maxHeightPercent);
        mPreviewMinMaxHeight.x = minHeightPercent / 100.0f;
        mPreviewMinMaxHeight.y = maxHeightPercent / 100.0f;
        modelView.setPreviewMinMaxHeight(mPreviewMinMaxHeight);
    }

    if(sidePane.drawButton("Preview")) {
        resetOverride();
        setOverride();
    }
}

void ExportAssistant::onToolSelect(ModelView& modelView) {
    modelView.setPreviewMinMaxHeight(mPreviewMinMaxHeight);
}

void ExportAssistant::onToolDeselect(ModelView& modelView) {
    resetOverride();
}

void ExportAssistant::resetOverride() {
    auto& modelView = mApplication.getModelView();
    modelView.setVertexNormalIndexOverride(false);
    modelView.getOverrideColorBuffer().clear();
    modelView.getOverrideIndexBuffer().clear();
    modelView.getOverrideNormalBuffer().clear();
    modelView.getOverrideVertexBuffer().clear();
    modelView.setPreviewMinMaxHeight(glm::vec2(0.0f, 1.0f));
}

void ExportAssistant::setOverride() {
    auto& modelView = mApplication.getModelView();
    std::vector<pepr3d::DataTriangle> triangles;
    // cube
    triangles.emplace_back(glm::vec3(-0.5, 0.5, -0.5), glm::vec3(-0.5, 0.5, 0.5), glm::vec3(0.5, 0.5, -0.5),
                           glm::vec3(0, 1, 0), 0);  // top
    triangles.emplace_back(glm::vec3(0.5, 0.5, -0.5), glm::vec3(0.5, 0.5, 0.5), glm::vec3(-0.5, 0.5, 0.5),
                           glm::vec3(0, 1, 0), 0);
    triangles.emplace_back(glm::vec3(-0.5, -0.5, -0.5), glm::vec3(-0.5, -0.5, 0.5), glm::vec3(0.5, -0.5, -0.5),
                           glm::vec3(0, -1, 0), 0);  // bottom
    triangles.emplace_back(glm::vec3(0.5, -0.5, -0.5), glm::vec3(0.5, -0.5, 0.5), glm::vec3(-0.5, -0.5, 0.5),
                           glm::vec3(0, -1, 0), 0);
    triangles.emplace_back(glm::vec3(0.5, -0.5, 0.5), glm::vec3(0.5, -0.5, -0.5), glm::vec3(0.5, 0.5, 0.5),
                           glm::vec3(1, 0, 0), 0);  // front
    triangles.emplace_back(glm::vec3(0.5, -0.5, -0.5), glm::vec3(0.5, 0.5, -0.5), glm::vec3(0.5, 0.5, 0.5),
                           glm::vec3(1, 0, 0), 0);
    triangles.emplace_back(glm::vec3(-0.5, -0.5, 0.5), glm::vec3(-0.5, -0.5, -0.5), glm::vec3(-0.5, 0.5, 0.5),
                           glm::vec3(-1, 0, 0), 0);  // back
    triangles.emplace_back(glm::vec3(-0.5, -0.5, -0.5), glm::vec3(-0.5, 0.5, -0.5), glm::vec3(-0.5, 0.5, 0.5),
                           glm::vec3(-1, 0, 0), 0);
    triangles.emplace_back(glm::vec3(-0.5, -0.5, 0.5), glm::vec3(0.5, -0.5, 0.5), glm::vec3(-0.5, 0.5, 0.5),
                           glm::vec3(0, 0, 1), 0);  // left
    triangles.emplace_back(glm::vec3(0.5, -0.5, 0.5), glm::vec3(0.5, 0.5, 0.5), glm::vec3(-0.5, 0.5, 0.5),
                           glm::vec3(0, 0, 1), 0);
    triangles.emplace_back(glm::vec3(-0.5, -0.5, -0.5), glm::vec3(0.5, -0.5, -0.5), glm::vec3(-0.5, 0.5, -0.5),
                           glm::vec3(0, 0, -1), 0);  // right
    triangles.emplace_back(glm::vec3(0.5, -0.5, -0.5), glm::vec3(0.5, 0.5, -0.5), glm::vec3(-0.5, 0.5, -0.5),
                           glm::vec3(0, 0, -1), 0);
    uint32_t index = 0;
    for(auto& tri : triangles) {
        modelView.getOverrideColorBuffer().push_back(
            mApplication.getCurrentGeometry()->getColorManager().getColor(tri.getColor()));
        modelView.getOverrideColorBuffer().push_back(
            mApplication.getCurrentGeometry()->getColorManager().getColor(tri.getColor()));
        modelView.getOverrideColorBuffer().push_back(
            mApplication.getCurrentGeometry()->getColorManager().getColor(tri.getColor()));
        modelView.getOverrideIndexBuffer().push_back(index++);
        modelView.getOverrideIndexBuffer().push_back(index++);
        modelView.getOverrideIndexBuffer().push_back(index++);
        modelView.getOverrideNormalBuffer().push_back(tri.getNormal());
        modelView.getOverrideNormalBuffer().push_back(tri.getNormal());
        modelView.getOverrideNormalBuffer().push_back(tri.getNormal());
        modelView.getOverrideVertexBuffer().push_back(tri.getVertex(0));
        modelView.getOverrideVertexBuffer().push_back(tri.getVertex(1));
        modelView.getOverrideVertexBuffer().push_back(tri.getVertex(2));
    }
    modelView.setVertexNormalIndexOverride(true);
}

}  // namespace pepr3d