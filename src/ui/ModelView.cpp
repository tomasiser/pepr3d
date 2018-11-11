#include "ModelView.h"
#include "MainApplication.h"
#include "geometry/Geometry.h"
#include "tools/Tool.h"

namespace pepr3d {

void ModelView::setup() {
    mCamera.lookAt(glm::vec3(3, 2, 2), glm::vec3(0, 0, 0));
    mCameraUi = ci::CameraUi(&mCamera);
    resize();

    mModelShader =
        ci::gl::GlslProg::create(ci::gl::GlslProg::Format()
                                     .vertex(ci::loadString(mApplication.loadAsset("shaders/ModelView.vert")))
                                     .fragment(ci::loadString(mApplication.loadAsset("shaders/ModelView.frag")))
                                     .attrib(ci::geom::Attrib::CUSTOM_0, "aColorIndex"));
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

    drawGeometry();

    {
        ci::gl::ScopedModelMatrix modelScope;
        ci::gl::multModelMatrix(glm::translate(glm::vec3(0, -0.5f, 0)));
        ci::gl::ScopedColor colorScope(ci::ColorA::black());
        ci::gl::ScopedLineWidth widthScope(1.0f);
        auto plane = ci::gl::Batch::create(ci::geom::WirePlane().subdivisions(glm::ivec2(16)),
                                           ci::gl::getStockShader(ci::gl::ShaderDef().color()));
        plane->draw();
    }

    {
        auto& currentTool = **mApplication.getCurrentToolIterator();
        currentTool.drawToModelView(*this);
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
    bool isMouseDown = event.isMiddleDown() || event.isRightDown();
    bool shouldPan = event.isControlDown() && isMouseDown;
    bool shouldTumble = !shouldPan && isMouseDown;
    mCameraUi.mouseDrag(event.getPos(), shouldTumble, shouldPan, false /* should zoom */);
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

void ModelView::onMouseMove(MouseEvent event) {
    auto* tool = mApplication.getCurrentTool();
    if(tool) {
        tool->onModelViewMouseMove(*this, event);
    }
}

ci::Ray ModelView::getRayFromWindowCoordinates(glm::ivec2 windowCoords) const {
    glm::vec2 viewportRelativeCoords(
        windowCoords.x / static_cast<float>(mViewport.second.x),
        1.0f - (windowCoords.y - mApplication.getToolbar().getHeight()) / static_cast<float>(mViewport.second.y));
    ci::Ray ray = mCamera.generateRay(viewportRelativeCoords.x, viewportRelativeCoords.y, mCamera.getAspectRatio());
    return ray;
}

void ModelView::drawGeometry() {
    if(mApplication.getCurrentGeometry() == nullptr) {
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
    const std::vector<Geometry::ColorIndex>& colors = mApplication.getCurrentGeometry()->getColorBuffer();
    assert(colors.size() == positions.size());

    const std::vector<glm::vec3>& normals = mApplication.getCurrentGeometry()->getNormalBuffer();

    // Create buffer layout
    const std::vector<cinder::gl::VboMesh::Layout> layout = {
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(ci::geom::Attrib::POSITION, 3),
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(ci::geom::Attrib::NORMAL, 3),
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(ci::geom::Attrib::CUSTOM_0, 1)};  // color index

    // Create elementary buffer of indices
    const cinder::gl::VboRef ibo = cinder::gl::Vbo::create(GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW);

    // Create the VBO mesh
    auto myVboMesh = ci::gl::VboMesh::create(static_cast<uint32_t>(positions.size()), GL_TRIANGLES, {layout},
                                             static_cast<uint32_t>(indices.size()), GL_UNSIGNED_INT, ibo);

    // Assign the buffers to the attributes
    myVboMesh->bufferAttrib<glm::vec3>(ci::geom::Attrib::POSITION, positions);
    myVboMesh->bufferAttrib<glm::vec3>(ci::geom::Attrib::NORMAL, normals);
    myVboMesh->bufferAttrib<Geometry::ColorIndex>(ci::geom::Attrib::CUSTOM_0, colors);

    // Assign color palette
    auto& colorMap = mApplication.getCurrentGeometry()->getColorManager().getColorMap();
    mModelShader->uniform("uColorPalette", &colorMap[0], static_cast<int>(colorMap.size()));
    mModelShader->uniform("uShowWireframe", mIsWireframeEnabled);

    // Create batch and draw
    auto myBatch = ci::gl::Batch::create(myVboMesh, mModelShader);
    myBatch->draw();
}

void ModelView::drawTriangleHighlight(const size_t triangleIndex) {
    auto geometry = mApplication.getCurrentGeometry();
    if(geometry == nullptr) {
        return;
    }

    auto triangle = geometry->getTriangle(triangleIndex);
    auto color = geometry->getColorManager().getColor(geometry->getTriangleColor(triangleIndex));
    float brightness = 0.2126f * color.r + 0.7152f * color.g + 0.0722f * color.b;
    bool isDarkHighlight = mIsWireframeEnabled ? (brightness <= 0.75f) : (brightness > 0.75f);
    ci::gl::ScopedColor drawColor(isDarkHighlight ? ci::ColorA::hex(0x1C2A35) : ci::ColorA::hex(0xFCFCFC));
    ci::gl::ScopedLineWidth drawWidth(mIsWireframeEnabled ? 3.0f : 1.0f);
    gl::ScopedDepth depth(false);
    ci::gl::drawLine(triangle.getVertex(0), triangle.getVertex(1));
    ci::gl::drawLine(triangle.getVertex(1), triangle.getVertex(2));
    ci::gl::drawLine(triangle.getVertex(2), triangle.getVertex(0));
    // ci::gl::drawSolidTriangle(triangle.getVertex(0), triangle.getVertex(1), triangle.getVertex(2));
}

}  // namespace pepr3d