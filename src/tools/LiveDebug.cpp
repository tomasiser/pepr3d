#include "tools/LiveDebug.h"
#include "geometry/Geometry.h"
#include "geometry/Triangle.h"
#include "imgui.h"
#include "ui/MainApplication.h"

namespace pepr3d {
using std::string;
using std::to_string;

void LiveDebug::drawToSidePane(SidePane& sidePane) {
    ImGui::BeginChild("##sidepane-livedebug");

    ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());
    if(sidePane.drawButton("Toggle ImGui demo window")) {
        mApplication.showDemoWindow(!mApplication.isDemoWindowShown());
    }
    ImGui::PopItemWidth();

    sidePane.drawSeparator();
    sidePane.drawText("Model view mouse pos:\nX: " + std::to_string(mMousePos.x) +
                      "\nY: " + std::to_string(mMousePos.y));

    const size_t triSize = mApplication.getCurrentGeometry()->getTriangleCount();
    sidePane.drawText("Vertex count: " + std::to_string(mApplication.getCurrentGeometry()->polyVertCount()) + "\n");
    sidePane.drawText("Triangle count: " + std::to_string(triSize) + "\n");
    sidePane.drawText("Polyhedron valid 0/1: " + std::to_string(mApplication.getCurrentGeometry()->polyhedronValid()) +
                      "\n");

    sidePane.drawSeparator();

    static int addedValue = 1;
    ImGui::Text("Current value: %i", mIntegerState.mInnerValue);
    if(mIntegerManager.canUndo()) {
        if(ImGui::Button("Undo")) {
            mIntegerManager.undo();
        }
    }
    if(mIntegerManager.canRedo()) {
        if(ImGui::Button("Redo")) {
            mIntegerManager.redo();
        }
    }
    ImGui::SliderInt("##addedvalue", &addedValue, 1, 10);
    ImGui::SameLine();
    if(ImGui::Button("Add")) {
        mIntegerManager.execute(std::make_unique<AddValueCommand>(addedValue));
    }

    ImGui::EndChild();
}

void LiveDebug::drawToModelView(ModelView& modelView) {
    if(mTriangleUnderRay && mApplication.getCurrentGeometry()->getTriangleCount() > mTriangleUnderRay) {
        modelView.drawTriangleHighlight(*mTriangleUnderRay);

        const auto& triangle = mApplication.getCurrentGeometry()->getTriangle(*mTriangleUnderRay);
        const glm::vec3 center = (triangle.getVertex(0) + triangle.getVertex(1) + triangle.getVertex(2)) / 3.f;

        const auto base1 = triangle.getTri().supporting_plane().base1();
        const auto base2 = triangle.getTri().supporting_plane().base2();

        modelView.drawLine(center, center + glm::vec3(base1.x(), base1.y(), base1.z()), ci::Color(1.f, 0.f, 0.f));
        modelView.drawLine(center, center + glm::vec3(base2.x(), base2.y(), base2.z()), ci::Color(0.f, 1.f, 0.f));
    }
}

void LiveDebug::onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event) {
    mMousePos = event.getPos();
    auto ray = modelView.getRayFromWindowCoordinates(event.getPos());
    mTriangleUnderRay = mApplication.getCurrentGeometry()->intersectMesh(ray);
}
}  // namespace pepr3d
