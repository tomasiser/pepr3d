#include "tools/TrianglePainter.h"
#include <random>
#include <vector>
#include "commands/CmdPaintSingleColor.h"
#include "geometry/Geometry.h"
#include "ui/MainApplication.h"

namespace pepr3d {

void TrianglePainter::drawToSidePane(SidePane& sidePane) {
    sidePane.drawColorPalette(mApplication.getCurrentGeometry()->getColorManager());
    sidePane.drawSeparator();
}

void TrianglePainter::drawToModelView(ModelView& modelView) {
    if(mHoveredTriangleId) {
        modelView.drawTriangleHighlight(*mHoveredTriangleId);
    }
}

void TrianglePainter::onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) {
    if(!event.isLeftDown()) {
        return;
    }
    mLastClick = event.getPos();
    if(!mHoveredTriangleId) {
        return;
    }
    auto geometry = mApplication.getCurrentGeometry();
    if(geometry == nullptr) {
        return;
    }

    // Avoid painting if the triangle is the same color
    const auto activeColor = geometry->getColorManager().getActiveColorIndex();
    if(geometry->getTriangleColor(*mHoveredTriangleId) != activeColor) {
        CommandManager<Geometry>* const commandManager = mApplication.getCommandManager();
        commandManager->execute(std::make_unique<CmdPaintSingleColor>(*mHoveredTriangleId, activeColor),
                                mGroupCommands);
        mGroupCommands = true;
    }
}

void TrianglePainter::onModelViewMouseUp(ModelView& modelView, ci::app::MouseEvent event) {
    mGroupCommands = false;
}

void TrianglePainter::onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event) {
    onModelViewMouseMove(modelView, event);
    onModelViewMouseDown(modelView, event);
}

void TrianglePainter::onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event) {
    mLastRay = modelView.getRayFromWindowCoordinates(event.getPos());
    auto geometry = mApplication.getCurrentGeometry();
    if(geometry == nullptr) {
        mHoveredTriangleId = {};
        return;
    }
    mHoveredTriangleId = safeIntersectMesh(mApplication, mLastRay);
}

void TrianglePainter::onNewGeometryLoaded(ModelView& modelView) {
    mHoveredTriangleId = {};
}

}  // namespace pepr3d