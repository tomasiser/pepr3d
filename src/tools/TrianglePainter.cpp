#include "tools/TrianglePainter.h"
#include <random>
#include "ui/MainApplication.h"

namespace pepr3d {

void TrianglePainter::drawToSidePane(SidePane& sidePane) {
    sidePane.drawText("Last click:\nX: " + std::to_string(mLastClick.x) + "\nY: " + std::to_string(mLastClick.y));
    const size_t triSize = mApplication.getCurrentGeometry()->getTriangleCount();
    sidePane.drawText("Number of triangles: " + std::to_string(triSize) + "\n");
    if(mSelectedTriangleId) {
        sidePane.drawText("Selected triangle ID: " + std::to_string(*mSelectedTriangleId) + "\n");
    }

    if(ImGui::Button("Color RED")) {
        mApplication.getCurrentGeometry()->setTriangleColor(*mSelectedTriangleId, 0);
        mSelectedTriangleOriginalColor = 0;
    }
    if(ImGui::Button("Color GREEN")) {
        mApplication.getCurrentGeometry()->setTriangleColor(*mSelectedTriangleId, 1);
        mSelectedTriangleOriginalColor = 1;
    }
    if(ImGui::Button("Color BLUE")) {
        mApplication.getCurrentGeometry()->setTriangleColor(*mSelectedTriangleId, 2);
        mSelectedTriangleOriginalColor = 2;
    }
    std::random_device rd;   // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd());  // Standard mersenne_twister_engine seeded with rd()
    uniform_real_distribution<> dis(0.0, 1.0);

    if(ImGui::Button("Randomize Colors")) {
        std::vector<ci::ColorA> newColors;
        for(int i = 0; i < 4; ++i) {
            newColors.emplace_back(dis(gen), dis(gen), dis(gen), 1);
        }
        mApplication.getCurrentGeometry()->replaceColors(std::move(newColors));
    }
}

void TrianglePainter::onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) {
    if(!event.isLeftDown()) {
        return;
    }
    mLastClick = event.getPos();
    mLastRay = modelView.getRayFromWindowCoordinates(event.getPos());
    auto geometry = mApplication.getCurrentGeometry();
    if(geometry == nullptr) {
        return;
    }
    auto previousTriangle = mSelectedTriangleId;
    mSelectedTriangleId = geometry->intersectMesh(mLastRay);
    if(previousTriangle == mSelectedTriangleId) {
        return;  // no change
    }
    if(previousTriangle) {
        geometry->setTriangleColor(*previousTriangle, mSelectedTriangleOriginalColor);
    }
    if(mSelectedTriangleId) {
        mSelectedTriangleOriginalColor = geometry->getTriangleColor(*mSelectedTriangleId);
        const size_t colorCount = geometry->getColorSize();
        /*auto inverseColor = ci::ColorA::white() - mSelectedTriangleOriginalColor;
        inverseColor[3] = 1.0f;*/

        geometry->setTriangleColor(*mSelectedTriangleId, colorCount - 1);
    }
}

void TrianglePainter::onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event) {
    onModelViewMouseDown(modelView, event);
}

}  // namespace pepr3d