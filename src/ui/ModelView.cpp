#include "ModelView.h"
#include "MainApplication.h"
#include "geometry/Geometry.h"
#include "tools/Tool.h"

namespace pepr3d {

void ModelView::setup() {
    resetCamera();
    mCameraUi = pepr3d::CameraUi(&mCamera);
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

    updateModelMatrix();

    drawGeometry();

    if(mIsGridEnabled) {
        ci::gl::ScopedModelMatrix modelScope;
        ci::gl::multModelMatrix(glm::scale(glm::vec3(0.9f)));  // i.e., new size is 2.0f * 0.9f = 1.8f
        ci::gl::ScopedColor colorScope(ci::ColorA::black());
        ci::gl::ScopedLineWidth widthScope(1.0f);
        auto plane = ci::gl::Batch::create(ci::geom::WirePlane().subdivisions(glm::ivec2(18)),  // i.e., 1 cell = 0.1f
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
    bool shouldPan = event.isMiddleDown() || (event.isControlDown() && event.isRightDown());
    bool shouldTumble = !shouldPan && event.isRightDown();
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

void ModelView::resetCamera() {
    mCamera.lookAt(glm::vec3(3.0f, 2.5f, 2.0f), glm::vec3(0.0f, 0.25f, 0.0f));
}

void ModelView::updateModelMatrix() {
    const Geometry* const geometry = mApplication.getCurrentGeometry();
    if(!geometry) {
        return;
    }
    const glm::vec3 aabbMin = geometry->getBoundingBoxMin();
    const glm::vec3 aabbMax = geometry->getBoundingBoxMax();
    const glm::vec3 aabbSize = aabbMax - aabbMin;
    const float maxSize = glm::max(glm::max(aabbSize.x, aabbSize.y), aabbSize.z);
    float inverseMaxSize = 1.0f / maxSize;

    if(std::isnan(inverseMaxSize)) {
        inverseMaxSize = 1.0f;  // fallback
    }

    // attention: in OpenGL, matrices are applied in reverse order (from the latest to the first)
    mModelMatrix = glm::scale(glm::vec3(inverseMaxSize, inverseMaxSize, -inverseMaxSize));
    mModelMatrix *= glm::translate(aabbSize / 2.0f);
    mModelMatrix *= glm::rotate(glm::radians(-90.0f), glm::vec3(1, 0, 0));
    mModelMatrix *= glm::translate(-aabbSize / 2.0f);
    mModelMatrix *= glm::translate(-aabbMin);

    const glm::vec4 aabbMinFit = mModelMatrix * glm::vec4(aabbMin, 1.0f);
    const glm::vec4 aabbMaxFit = mModelMatrix * glm::vec4(aabbMax, 1.0f);
    const glm::vec4 aabbSizeFit = aabbMaxFit - aabbMinFit;

    mModelMatrix =
        glm::translate(glm::vec3(-aabbSizeFit.x / 2.0f, -aabbSizeFit.z / 2.0f, aabbSizeFit.y / 2.0f)) * mModelMatrix;
    mModelMatrix = glm::rotate(glm::radians(mModelRoll), glm::vec3(1, 0, 0)) * mModelMatrix;
    mModelMatrix = glm::translate(glm::vec3(0.0f, aabbSizeFit.z / 2.0f - aabbMaxFit.z, 0.0f)) * mModelMatrix;
    mModelMatrix = glm::translate(mModelTranslate) * mModelMatrix;
}

ci::Ray ModelView::getRayFromWindowCoordinates(glm::ivec2 windowCoords) const {
    glm::vec2 viewportRelativeCoords(
        windowCoords.x / static_cast<float>(mViewport.second.x),
        1.0f - (windowCoords.y - mApplication.getToolbar().getHeight()) / static_cast<float>(mViewport.second.y));
    ci::Ray ray = mCamera.generateRay(viewportRelativeCoords.x, viewportRelativeCoords.y, mCamera.getAspectRatio());
    glm::mat4 inverseModelMatrix = glm::inverse(mModelMatrix);
    ray.setOrigin(inverseModelMatrix * glm::vec4(ray.getOrigin(), 1));                        // point, w=1
    ray.setDirection(glm::normalize(inverseModelMatrix * glm::vec4(ray.getDirection(), 0)));  // vector, w=0
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

    assert(!mColorOverride.isOverriden || mColorOverride.overrideColorBuffer.size() == positions.size());

    const std::vector<glm::vec3>& normals = mApplication.getCurrentGeometry()->getNormalBuffer();

    // Create buffer layout
    const std::vector<cinder::gl::VboMesh::Layout> layout = {
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(ci::geom::Attrib::POSITION, 3),
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(ci::geom::Attrib::NORMAL, 3),
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(ci::geom::Attrib::COLOR, 4),
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(ci::geom::Attrib::CUSTOM_0, 1)};  // color index

    // Create elementary buffer of indices
    const cinder::gl::VboRef ibo = cinder::gl::Vbo::create(GL_ELEMENT_ARRAY_BUFFER, indices, GL_STATIC_DRAW);

    // Create the VBO mesh
    auto myVboMesh = ci::gl::VboMesh::create(static_cast<uint32_t>(positions.size()), GL_TRIANGLES, {layout},
                                             static_cast<uint32_t>(indices.size()), GL_UNSIGNED_INT, ibo);

    // Assign the buffers to the attributes
    myVboMesh->bufferAttrib<glm::vec3>(ci::geom::Attrib::POSITION, positions);
    myVboMesh->bufferAttrib<glm::vec3>(ci::geom::Attrib::NORMAL, normals);
    myVboMesh->bufferAttrib<glm::vec4>(ci::geom::Attrib::COLOR, mColorOverride.overrideColorBuffer);
    myVboMesh->bufferAttrib<Geometry::ColorIndex>(ci::geom::Attrib::CUSTOM_0, colors);

    // Assign color palette
    auto& colorMap = mApplication.getCurrentGeometry()->getColorManager().getColorMap();
    mModelShader->uniform("uColorPalette", &colorMap[0], static_cast<int>(colorMap.size()));
    mModelShader->uniform("uShowWireframe", mIsWireframeEnabled);
    mModelShader->uniform("uOverridePalette", mColorOverride.isOverriden);

    const ci::gl::ScopedModelMatrix scopedModelMatrix;
    ci::gl::multModelMatrix(mModelMatrix);

    // Create batch and draw
    auto myBatch = ci::gl::Batch::create(myVboMesh, mModelShader);
    myBatch->draw();
}

void ModelView::drawTriangleHighlight(const size_t triangleIndex) {
    const Geometry* const geometry = mApplication.getCurrentGeometry();
    if(geometry == nullptr || triangleIndex >= geometry->getTriangleCount()) {
        return;
    }

    const ci::gl::ScopedModelMatrix scopedModelMatrix;
    ci::gl::multModelMatrix(mModelMatrix);

    const DataTriangle& triangle = geometry->getTriangle(triangleIndex);
    const glm::vec4 color = geometry->getColorManager().getColor(geometry->getTriangleColor(triangleIndex));
    const float brightness = 0.2126f * color.r + 0.7152f * color.g + 0.0722f * color.b;
    const bool isDarkHighlight = mIsWireframeEnabled ? (brightness <= 0.75f) : (brightness > 0.75f);
    ci::gl::ScopedColor drawColor(isDarkHighlight ? ci::ColorA::hex(0x1C2A35) : ci::ColorA::hex(0xFCFCFC));
    ci::gl::ScopedLineWidth drawWidth(mIsWireframeEnabled ? 3.0f : 1.0f);
    gl::ScopedDepth depth(false);
    ci::gl::drawLine(triangle.getVertex(0), triangle.getVertex(1));
    ci::gl::drawLine(triangle.getVertex(1), triangle.getVertex(2));
    ci::gl::drawLine(triangle.getVertex(2), triangle.getVertex(0));
}

}  // namespace pepr3d