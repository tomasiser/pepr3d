#pragma once

#include "cinder/gl/gl.h"

#include "CinderImGui.h"
#include "UiStateStore.h"

namespace pepr3d {

inline void setupModelView(UiStateStore& state) {
    ModelViewState& modelView = state.modelView;

    modelView.viewport = std::make_pair(glm::ivec2(0), glm::ivec2(state.mainWindow.size.x - state.sidePane.width,
                                                                  state.mainWindow.size.y - state.toolbar.height));
    modelView.camera.setAspectRatio(static_cast<float>(modelView.viewport.second.x) / modelView.viewport.second.y);
    modelView.camera.lookAt(glm::vec3(3, 2, 2), glm::vec3(0, 0, 0));
}

inline void onModelViewMouseDown(UiStateStore& state, ci::app::MouseEvent event) {}

inline void onModelViewMouseDrag(UiStateStore& state, ci::app::MouseEvent event) {}

inline void drawModelView(UiStateStore& state) {
    const auto originalViewport = ci::gl::getViewport();
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

    const auto lambert = ci::gl::ShaderDef().lambert().color();
    const auto shader = ci::gl::getStockShader(lambert);

    // ---------- Draw the model ----------
    // ci::gl::color(ci::ColorA::hex(0xDA017B));
    ci::gl::color(color[0], color[1], color[2]);

    // Get vertex buffer
    const std::vector<glm::vec3> positions = state.geo.getVertexBuffer();
    // Get index buffer
    std::vector<uint32_t> indices = state.geo.getIndexBuffer();
    // \todo maybe get only iterators, or references to the buffers?
    // Since we use a new vertex for each triangle, we should have vertices == triangles
    assert(indices.size() == positions.size());

    // Generate a color buffer (demo)
    const int numVerts = positions.size();
    std::vector<cinder::ColorA> colors;
    for(size_t k = 0; k < numVerts; k++) {
        colors.emplace_back(1.0, 0.0, 0.0, 1.0);
    }

    // Create buffer layout
    const std::vector<cinder::gl::VboMesh::Layout> layout = {
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(ci::geom::Attrib::POSITION, 3),
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(ci::geom::Attrib::COLOR, 4)};

    // Create elementary buffer of indices
    cinder::gl::VboRef ibo = cinder::gl::Vbo::create(GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW);

    // Create the VBO mesh
    auto myVboMesh = ci::gl::VboMesh::create(numVerts, GL_TRIANGLES, {layout}, indices.size(), GL_UNSIGNED_INT, ibo);

    // Assign the buffers to the attributes
    myVboMesh->bufferAttrib<glm::vec3>(ci::geom::Attrib::POSITION, positions);
    myVboMesh->bufferAttrib<cinder::ColorA>(ci::geom::Attrib::COLOR, colors);

    // Create batch and draw
    auto myBatch = ci::gl::Batch::create(myVboMesh, ci::gl::getStockShader(ci::gl::ShaderDef().color()));
    myBatch->draw();

    // ---------- Draw the wireframe plane ----------
    auto plane = ci::gl::Batch::create(ci::geom::WirePlane().subdivisions(glm::ivec2(16)), shader);
    ci::gl::translate(0, -0.5f, 0);
    plane->draw();

    ci::gl::popMatrices();

    ci::gl::viewport(originalViewport);
}

}  // namespace pepr3d