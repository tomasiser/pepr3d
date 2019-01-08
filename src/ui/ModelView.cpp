#include "ModelView.h"
#include "MainApplication.h"
#include "geometry/Geometry.h"
#include "tools/Tool.h"

namespace pepr3d {
using namespace ci;

void ModelView::setup() {
    resetCamera();
    mCameraUi = pepr3d::CameraUi(&mCamera);
    resize();

    mModelShader =
        ci::gl::GlslProg::create(ci::gl::GlslProg::Format()
                                     .vertex(ci::loadString(mApplication.loadAsset("shaders/ModelView.vert")))
                                     .fragment(ci::loadString(mApplication.loadAsset("shaders/ModelView.frag")))
                                     .attrib(Attributes::COLOR_IDX, "aColorIndex")
                                     .attrib(Attributes::HIGHLIGHT_MASK, "aAreaHighlightMask"));
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
        ci::gl::multModelMatrix(glm::translate(glm::vec3(0.0f, mGridOffset, 0.0f)) *
                                glm::scale(glm::vec3(0.9f)));  // i.e., new size is 2.0f * 0.9f = 1.8f
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
    mCamera.lookAt(glm::vec3(2.4f, 1.8f, 1.6f), glm::vec3(0.0f, 0.0f, 0.0f));
}

void ModelView::updateVboAndBatch() {
    const Geometry::OpenGlData& glData = mApplication.getCurrentGeometry()->getOpenGlData();
    assert(!glData.isDirty);

    // Create buffer layout
    const std::vector<cinder::gl::VboMesh::Layout> layout = {
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(ci::geom::Attrib::POSITION, 3),
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(ci::geom::Attrib::NORMAL, 3),
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(ci::geom::Attrib::COLOR, 4),
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(Attributes::COLOR_IDX, 1),
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(Attributes::HIGHLIGHT_MASK, 1)};

    // Create elementary buffer of indices
    const cinder::gl::VboRef ibo = cinder::gl::Vbo::create(GL_ELEMENT_ARRAY_BUFFER, glData.indexBuffer, GL_STATIC_DRAW);

    // Create the VBO mesh
    mVboMesh = ci::gl::VboMesh::create(static_cast<uint32_t>(glData.vertexBuffer.size()), GL_TRIANGLES, {layout},
                                       static_cast<uint32_t>(glData.vertexBuffer.size()), GL_UNSIGNED_INT, ibo);

    // Assign the buffers to the attributes
    mVboMesh->bufferAttrib<glm::vec3>(ci::geom::Attrib::POSITION, glData.vertexBuffer);
    mVboMesh->bufferAttrib<glm::vec3>(ci::geom::Attrib::NORMAL, glData.normalBuffer);
    mVboMesh->bufferAttrib<glm::vec4>(ci::geom::Attrib::COLOR, mColorOverride.overrideColorBuffer);
    mVboMesh->bufferAttrib<Geometry::ColorIndex>(Attributes::COLOR_IDX, glData.colorBuffer);
    mVboMesh->bufferAttrib<GLint>(Attributes::HIGHLIGHT_MASK, glData.highlightMask);

    mBatch = ci::gl::Batch::create(mVboMesh, mModelShader);
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
    mMaxSize = maxSize;
    float inverseMaxSize = 1.0f / maxSize;

    if(std::isnan(inverseMaxSize)) {
        inverseMaxSize = 1.0f;  // fallback
    }

    // attention: in OpenGL, matrices are applied in reverse order (from the latest to the first)
    mModelMatrix = glm::scale(glm::vec3(inverseMaxSize, inverseMaxSize, inverseMaxSize));
    mModelMatrix *= glm::translate(aabbSize / 2.0f);
    mModelMatrix *= glm::rotate(glm::radians(-90.0f), glm::vec3(1, 0, 0));
    mModelMatrix *= glm::translate(-aabbSize / 2.0f);
    mModelMatrix *= glm::translate(-aabbMin);

    const glm::vec4 aabbMinFit = mModelMatrix * glm::vec4(aabbMin, 1.0f);
    const glm::vec4 aabbMaxFit = mModelMatrix * glm::vec4(aabbMax, 1.0f);
    const glm::vec4 aabbSizeFit = aabbMaxFit - aabbMinFit;

    mModelMatrix =
        glm::translate(glm::vec3(-aabbSizeFit.x / 2.0f, aabbSizeFit.z / 2.0f, -aabbSizeFit.y / 2.0f)) * mModelMatrix;
    mModelMatrix = glm::rotate(glm::radians(mModelRoll), glm::vec3(1, 0, 0)) * mModelMatrix;
    mModelMatrix = glm::translate(mModelTranslate) * mModelMatrix;

    mGridOffset = -(aabbMaxFit.z - aabbSizeFit.z / 2.0f);
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

    const Geometry::OpenGlData& glData = mApplication.getCurrentGeometry()->getOpenGlData();
    if(glData.isDirty || !mBatch) {
        mApplication.getCurrentGeometry()->updateOpenGlBuffers();
        updateVboAndBatch();

        CI_LOG_I("Vbo updated");
    }

    // Pass new highlight data if required
    if(glData.info.didHighlightUpdate) {
        mVboMesh->bufferAttrib<GLint>(Attributes::HIGHLIGHT_MASK, glData.highlightMask);
        glData.info.unsetHighlightFlag();
    }

    // Pass new color data if required
    if(glData.info.didColorUpdate) {
        mVboMesh->bufferAttrib<Geometry::ColorIndex>(Attributes::COLOR_IDX, glData.colorBuffer);
        glData.info.unsetColorFlag();
    }

    assert(!mColorOverride.isOverriden || mColorOverride.overrideColorBuffer.size() == glData.vertexBuffer.size());

    // Pass overriden colors if required
    if(mColorOverride.isOverriden) {
        mVboMesh->bufferAttrib<glm::vec4>(ci::geom::Attrib::COLOR, mColorOverride.overrideColorBuffer);
    }

    // Assign color palette
    auto& colorMap = mApplication.getCurrentGeometry()->getColorManager().getColorMap();
    mModelShader->uniform("uColorPalette", &colorMap[0], static_cast<int>(colorMap.size()));
    mModelShader->uniform("uShowWireframe", mIsWireframeEnabled);
    mModelShader->uniform("uOverridePalette", mColorOverride.isOverriden);

    const ci::gl::ScopedModelMatrix scopedModelMatrix;
    ci::gl::multModelMatrix(mModelMatrix);

    // Assign highlight uniforms
    auto& areaHighlight = mApplication.getCurrentGeometry()->getAreaHighlight();
    mModelShader->uniform("uAreaHighlightEnabled", areaHighlight.enabled);
    mModelShader->uniform("uAreaHighlightOrigin", areaHighlight.origin - mModelTranslate);
    mModelShader->uniform("uAreaHighlightSize", static_cast<float>(areaHighlight.size));
    mModelShader->uniform("uAreaHighlightColor", vec3(0.f, 1.f, 0.f));

    mBatch->draw();
}

void ModelView::drawTriangleHighlight(const DetailedTriangleId triangleId) {
    const Geometry* const geometry = mApplication.getCurrentGeometry();
    if(geometry == nullptr || triangleId.getBaseId() >= geometry->getTriangleCount()) {
        return;
    }

    if(triangleId.getDetailId() && triangleId.getDetailId() >= geometry->getTriangleDetailCount(triangleId.getBaseId()))
    {
        return;
    }

    const ci::gl::ScopedModelMatrix scopedModelMatrix;
    ci::gl::multModelMatrix(mModelMatrix);

    const DataTriangle& triangle = geometry->getTriangle(triangleId);
    const glm::vec4 color = geometry->getColorManager().getColor(geometry->getTriangleColor(triangleId));
    const float brightness = 0.2126f * color.r + 0.7152f * color.g + 0.0722f * color.b;
    const bool isDarkHighlight = mIsWireframeEnabled ? (brightness <= 0.75f) : (brightness > 0.75f);
    ci::gl::ScopedColor drawColor(isDarkHighlight ? ci::ColorA::hex(0x1C2A35) : ci::ColorA::hex(0xFCFCFC));
    ci::gl::ScopedLineWidth drawWidth(mIsWireframeEnabled ? 3.0f : 1.0f);
    gl::ScopedDepth depth(false);
    ci::gl::drawLine(triangle.getVertex(0), triangle.getVertex(1));
    ci::gl::drawLine(triangle.getVertex(1), triangle.getVertex(2));
    ci::gl::drawLine(triangle.getVertex(2), triangle.getVertex(0));
}

void ModelView::drawLine(const glm::vec3& from, const glm::vec3& to, const ci::Color& color) {
    const ci::gl::ScopedModelMatrix scopedModelMatrix;
    ci::gl::multModelMatrix(mModelMatrix);
    ci::gl::ScopedColor drawColor(color);
    ci::gl::ScopedLineWidth drawWidth(mIsWireframeEnabled ? 3.0f : 1.0f);
    gl::ScopedDepth depth(false);
    ci::gl::drawLine(from, to);
}

}  // namespace pepr3d