#pragma once

#include "cinder/gl/gl.h"

#include "CinderImGui.h"
#include "UiStateStore.h"

namespace pepr3d {

inline void onModelViewResize(UiStateStore& state) {
    ModelViewState& modelView = state.modelView;
    modelView.viewport = std::make_pair(glm::ivec2(0), glm::ivec2(state.mainWindow.size.x - state.sidePane.width,
                                                                  state.mainWindow.size.y - state.toolbar.height));
    modelView.camera.setAspectRatio(static_cast<float>(modelView.viewport.second.x) / modelView.viewport.second.y);
}

inline void setupModelView(UiStateStore& state) {
    ModelViewState& modelView = state.modelView;
    onModelViewResize(state);
    modelView.camera.lookAt(glm::vec3(3, 2, 2), glm::vec3(0, 0, 0));
}

inline void onModelViewMouseDown(UiStateStore& state, ci::app::MouseEvent event) {}

inline void onModelViewMouseDrag(UiStateStore& state, ci::app::MouseEvent event) {}

inline void drawModelView(UiStateStore& state) {
    auto originalViewport = ci::gl::getViewport();
    ci::gl::viewport(state.modelView.viewport);
    ci::gl::enableDepthRead();
    ci::gl::enableDepthWrite();

    state.modelView.camera.setAspectRatio(static_cast<float>(state.modelView.viewport.second.x) /
                                          state.modelView.viewport.second.y);

    ci::gl::pushMatrices();
    ci::gl::setMatrices(state.modelView.camera);

    ImGui::Begin("##sidepane-debug");
    static float color[] = {255.f / 256.f, 134.f / 256.f, 37.f / 256.f};
    ImGui::ColorPicker3("##objectcolor", color);
    ImGui::End();

    auto lambert = ci::gl::ShaderDef().lambert().color();
    auto shader = ci::gl::getStockShader(lambert);
    auto cube = ci::gl::Batch::create(ci::geom::Cube(), shader);
    // ci::gl::color(ci::ColorA::hex(0xDA017B));
    ci::gl::color(color[0], color[1], color[2]);
    cube->draw();
    auto plane = ci::gl::Batch::create(ci::geom::WirePlane().subdivisions(glm::ivec2(16)), shader);
    ci::gl::translate(0, -0.5f, 0);
    plane->draw();

    ci::gl::popMatrices();

    ci::gl::viewport(originalViewport);
}

}  // namespace pepr3d