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

    ImGui::Begin("##sidepane-debug");
    static float color[] = {255.f / 256.f, 134.f / 256.f, 37.f / 256.f};
    ImGui::ColorPicker3("##objectcolor", color);
    ImGui::End();

    auto lambert = ci::gl::ShaderDef().lambert().color();
    auto shader = ci::gl::getStockShader(lambert);
    // ci::gl::color(ci::ColorA::hex(0xDA017B));
    ci::gl::color(color[0], color[1], color[2]);
    drawGeometry();
    auto plane = ci::gl::Batch::create(ci::geom::WirePlane().subdivisions(glm::ivec2(16)), shader);
    ci::gl::translate(0, -0.5f, 0);
    plane->draw();

    ci::gl::popMatrices();

    ci::gl::viewport(originalViewport);
}

void ModelView::drawGeometry() {
    if (mApplication.getCurrentGeometry() == nullptr) {
        return;
    }

    // \todo Getting const references to the buffers, might want to use pointers, iterators, whatever.
    // Get vertex buffer
    const std::vector<glm::vec3>& positions = mApplication.getCurrentGeometry()->getVertexBuffer();
    // Get index buffer
    const std::vector<uint32_t>& indices = mApplication.getCurrentGeometry()->getIndexBuffer();
    // Since we use a new vertex for each triangle, we should have vertices == triangles
    assert(indices.size() == positions.size());
    // Get the color buffer
    const std::vector<cinder::ColorA>& colors = mApplication.getCurrentGeometry()->getColorBuffer();
    assert(colors.size() == positions.size());

    const std::vector<glm::vec3>& normals = mApplication.getCurrentGeometry()->getNormalBuffer();

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
    auto myBatch = ci::gl::Batch::create(myVboMesh, ci::gl::getStockShader(ci::gl::ShaderDef().lambert().color()));
    myBatch->draw();
}

}  // namespace pepr3d