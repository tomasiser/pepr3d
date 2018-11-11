#include "tools/TrianglePainter.h"
#include <random>
#include <vector>
#include "geometry/Geometry.h"
#include "ui/MainApplication.h"

namespace pepr3d {

class CmdPaintSingleTriangle : public CommandBase<Geometry> {
   public:
    std::string_view getDescription() const override {
        return "Set color of a triangle";
    }

    CmdPaintSingleTriangle(size_t triangleId, size_t colorId)
        : CommandBase(false, 0xC4E9741F2935D241), mTriangleIds{triangleId}, mColorId(colorId) {}

   protected:
    void run(Geometry& target) const override {
        for(size_t triangleId : mTriangleIds) {
            target.setTriangleColor(triangleId, mColorId);
        }
    }

    bool joinCommand(const CommandBase& otherBase) override {
        const auto* other = dynamic_cast<const CmdPaintSingleTriangle*>(&otherBase);
        if(other) {
            mTriangleIds.insert(mTriangleIds.end(), other->mTriangleIds.begin(), other->mTriangleIds.end());
            return true;
        } else {
            return false;
        }
    }

    std::vector<size_t> mTriangleIds;
    size_t mColorId;
};

void TrianglePainter::drawToSidePane(SidePane& sidePane) {
    sidePane.drawColorPalette(mApplication.getCurrentGeometry()->getColorManager());
    sidePane.drawSeparator();

    sidePane.drawText("Last click:\nX: " + std::to_string(mLastClick.x) + "\nY: " + std::to_string(mLastClick.y));
    const size_t triSize = mApplication.getCurrentGeometry()->getTriangleCount();
    sidePane.drawText("Number of triangles: " + std::to_string(triSize) + "\n");
    if(mHoveredTriangleId) {
        sidePane.drawText("Hovered triangle ID: " + std::to_string(*mHoveredTriangleId) + "\n");
    }
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
        mCommandManager.execute(std::make_unique<CmdPaintSingleTriangle>(*mHoveredTriangleId, activeColor),
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
    mHoveredTriangleId = geometry->intersectMesh(mLastRay);
}

}  // namespace pepr3d