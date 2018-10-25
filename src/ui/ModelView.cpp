#include "ModelView.h"
#include "MainApplication.h"

namespace pepr3d {

void ModelView::setup() {
    mCamera.lookAt(glm::vec3(3, 2, 2), glm::vec3(0, 0, 0));
    mCameraUi = ci::CameraUi(&mCamera);
    resize();
}

void ModelView::resize() {
    auto windowSize = mApplication.getWindowSize();
    mViewport = std::make_pair(glm::ivec2(0), glm::ivec2(windowSize.x - mApplication.getSidePane().getWidth(),
                                                         windowSize.y - mApplication.getToolbar().getHeight()));
    mCameraUi.setWindowSize(mViewport.second);
    mCamera.setAspectRatio(static_cast<float>(mViewport.second.x) / mViewport.second.y);
}

void ModelView::draw() {
    ci::gl::ScopedViewport viewport(mViewport.first, mViewport.second);

    gl::ScopedMatrices push;
    ci::gl::setMatrices(mCamera);
    gl::ScopedDepth depth(true);

    {
        auto lambert = ci::gl::ShaderDef().lambert().color();
        auto shader = ci::gl::getStockShader(lambert);
        auto cube = ci::gl::Batch::create(ci::geom::Cube(), shader);
        ci::gl::ScopedColor colorScope(ci::ColorA::hex(0xDA017B));
        cube->draw();
    }

    {
        ci::gl::ScopedModelMatrix modelScope;
        ci::gl::multModelMatrix(glm::translate(glm::vec3(0, -0.5f, 0)));
        ci::gl::ScopedColor colorScope(ci::ColorA::black());
        auto plane = ci::gl::Batch::create(ci::geom::WirePlane().subdivisions(glm::ivec2(16)),
                                           ci::gl::getStockShader(ci::gl::ShaderDef().color()));
        plane->draw();
    }
}

void ModelView::onMouseDown(MouseEvent event) {
    auto* tool = mApplication.getCurrentTool();
    if(tool) {
        tool->onModelViewMouseDown(*this, event);
    }

    mCameraUi.mouseDown(event.getPos());
}

void ModelView::onMouseDrag(MouseEvent event) {
    auto* tool = mApplication.getCurrentTool();
    if(tool) {
        tool->onModelViewMouseDrag(*this, event);
    }
    mCameraUi.mouseDrag(event.getPos(), event.isRightDown() || event.isMiddleDown(), false, false);
}

void ModelView::onMouseUp(MouseEvent event) {
    auto* tool = mApplication.getCurrentTool();
    if(tool) {
        tool->onModelViewMouseUp(*this, event);
    }
    mCameraUi.mouseUp(event.getPos());
}

void ModelView::onMouseWheel(MouseEvent event) {
    auto* tool = mApplication.getCurrentTool();
    if(tool) {
        tool->onModelViewMouseWheel(*this, event);
    }
    mCameraUi.mouseWheel(-event.getWheelIncrement());
}

ci::Ray ModelView::getRayFromWindowCoordinates(glm::ivec2 windowCoords) const {
    glm::vec2 viewportRelativeCoords(
        windowCoords.x / static_cast<float>(mViewport.second.x),
        1.0f - (windowCoords.y - mApplication.getToolbar().getHeight()) / static_cast<float>(mViewport.second.y));
    ci::Ray ray = mCamera.generateRay(viewportRelativeCoords.x, viewportRelativeCoords.y, mCamera.getAspectRatio());
    return ray;
}

}  // namespace pepr3d