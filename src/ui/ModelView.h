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

inline void drawModelMesh(UiStateStore& state, const cinder::gl::GlslProgRef& shader) {
    // \todo Getting const references to the buffers, might want to use pointers, iterators, whatever.
    // Get vertex buffer
    const std::vector<glm::vec3>& positions = state.geometryData.getVertexBuffer();
    // Get index buffer
    const std::vector<uint32_t>& indices = state.geometryData.getIndexBuffer();
    // Since we use a new vertex for each triangle, we should have vertices == triangles
    assert(indices.size() == positions.size());
    // Get the color buffer
    const std::vector<cinder::ColorA>& colors = state.geometryData.getColorBuffer();
    assert(colors.size() == positions.size());

    const std::vector<glm::vec3>& normals = state.geometryData.getNormalBuffer();

    // Create buffer layout
    const std::vector<cinder::gl::VboMesh::Layout> layout = {
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(ci::geom::Attrib::POSITION, 3),
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(ci::geom::Attrib::NORMAL, 3),
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(ci::geom::Attrib::COLOR, 4)};

    // Create elementary buffer of indices
    const cinder::gl::VboRef ibo = cinder::gl::Vbo::create(GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW);

    // Create the VBO mesh
    auto myVboMesh = ci::gl::VboMesh::create(static_cast<uint32_t>(positions.size()), GL_TRIANGLES, {layout},
                                             static_cast<uint32_t>(indices.size()), GL_UNSIGNED_INT, ibo);

    // Assign the buffers to the attributes
    myVboMesh->bufferAttrib<glm::vec3>(ci::geom::Attrib::POSITION, positions);
    myVboMesh->bufferAttrib<glm::vec3>(ci::geom::Attrib::NORMAL, normals);
    myVboMesh->bufferAttrib<cinder::ColorA>(ci::geom::Attrib::COLOR, colors);

    // Create batch and draw
    auto myBatch = ci::gl::Batch::create(myVboMesh, shader);
    myBatch->draw();
}

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

    // Draw the model
    // ci::gl::color(ci::ColorA::hex(0xDA017B));
    ci::gl::color(color[0], color[1], color[2]);
    drawModelMesh(state, shader);

    // Draw the wireframe plane
    auto plane = ci::gl::Batch::create(ci::geom::WirePlane().subdivisions(glm::ivec2(16)), shader);
    ci::gl::translate(0, -0.5f, 0);
    plane->draw();

    ci::gl::popMatrices();

    ci::gl::viewport(originalViewport);
}

}  // namespace pepr3d