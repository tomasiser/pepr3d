#include "ModelView.h"
#include "MainApplication.h"

namespace pepr3d {

void ModelView::setup() {
    mCamera.lookAt(glm::vec3(3, 2, 2), glm::vec3(0, 0, 0));
    mCameraUi = ci::CameraUi(&mCamera, mApplication.getWindow());
    mCameraUi.setMouseWheelMultiplier(-mCameraUi.getMouseWheelMultiplier());
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
    auto originalViewport = ci::gl::getViewport();
    ci::gl::viewport(mViewport);
    ci::gl::enableDepthRead();
    ci::gl::enableDepthWrite();

    ci::gl::pushMatrices();
    ci::gl::setMatrices(mCamera);

    // ImGui::Begin("##sidepane-livedebug");
    static float color[] = {255.f / 256.f, 134.f / 256.f, 37.f / 256.f};
    // ImGui::ColorPicker3("##objectcolor", color);
    // ImGui::End();

    auto lambert = ci::gl::ShaderDef().lambert().color();
    auto shader = ci::gl::getStockShader(lambert);
    auto cube = ci::gl::Batch::create(ci::geom::Cube(), shader);
    // ci::gl::color(ci::ColorA::hex(0xDA017B));
    ci::gl::color(color[0], color[1], color[2]);
    cube->draw();
    auto plane = ci::gl::Batch::create(ci::geom::WirePlane().subdivisions(glm::ivec2(16)), ci::gl::getStockShader(ci::gl::ShaderDef().color()));
    ci::gl::color(ci::ColorA::black());
    ci::gl::translate(0, -0.5f, 0);
    plane->draw();

    ci::gl::popMatrices();

    ci::gl::viewport(originalViewport);
}

}  // namespace pepr3d