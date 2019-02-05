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
                                     .vertex(ci::loadString(mApplication.loadRequiredAsset("shaders/ModelView.vert")))
                                     .fragment(ci::loadString(mApplication.loadRequiredAsset("shaders/ModelView.frag")))
                                     .attrib(Attributes::COLOR_IDX, "aColorIndex")
                                     .attrib(Attributes::HIGHLIGHT_MASK, "aAreaHighlightMask"));
    mModelShader->uniform("uPreviewMinMaxHeight", mPreviewMinMaxHeight);
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
        // draw dummy window:
        // (necessary for drawing texts into the model view)
        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_NoTitleBar;
        window_flags |= ImGuiWindowFlags_NoScrollbar;
        window_flags |= ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoResize;
        window_flags |= ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoNav;
        window_flags |= ImGuiWindowFlags_NoSavedSettings;
        window_flags |= ImGuiWindowFlags_NoInputs;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, glm::vec4(0.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, glm::vec4(0.0f));
        ImGui::Begin("##modelview-dummy", nullptr, window_flags);

        // let the active tool draw to the model view:
        auto& currentTool = **mApplication.getCurrentToolIterator();
        currentTool.drawToModelView(*this);

        // end the dummy window:
        ImGui::End();
        ImGui::PopStyleColor(2);
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
    mCamera.setFov(35.0f);
}

void ModelView::updateVboAndBatch() {
    const Geometry::OpenGlData& glData = mApplication.getCurrentGeometry()->getOpenGlData();
    assert(isVertexNormalIndexOverride() || !glData.isDirty);

    // Create buffer layout
    const std::vector<cinder::gl::VboMesh::Layout> layout = {
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(ci::geom::Attrib::POSITION, 3),
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(ci::geom::Attrib::NORMAL, 3),
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(ci::geom::Attrib::COLOR, 4),
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(Attributes::COLOR_IDX, 1),
        cinder::gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(Attributes::HIGHLIGHT_MASK, 1)};

    // Create elementary buffer of indices
    const cinder::gl::VboRef ibo = cinder::gl::Vbo::create(
        GL_ELEMENT_ARRAY_BUFFER, isVertexNormalIndexOverride() ? getOverrideIndexBuffer() : glData.indexBuffer,
        GL_STATIC_DRAW);

    // Create the VBO mesh
    mVboMesh =
        ci::gl::VboMesh::create(static_cast<uint32_t>(isVertexNormalIndexOverride() ? getOverrideVertexBuffer().size()
                                                                                    : glData.vertexBuffer.size()),
                                GL_TRIANGLES, {layout},
                                static_cast<uint32_t>(isVertexNormalIndexOverride() ? getOverrideVertexBuffer().size()
                                                                                    : glData.vertexBuffer.size()),
                                GL_UNSIGNED_INT, ibo);

    // Assign the buffers to the attributes
    mVboMesh->bufferAttrib<glm::vec3>(ci::geom::Attrib::POSITION,
                                      isVertexNormalIndexOverride() ? getOverrideVertexBuffer() : glData.vertexBuffer);
    mVboMesh->bufferAttrib<glm::vec3>(ci::geom::Attrib::NORMAL,
                                      isVertexNormalIndexOverride() ? getOverrideNormalBuffer() : glData.normalBuffer);
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

    glm::vec4 aabbMinFit = mModelMatrix * glm::vec4(aabbMin, 1.0f);
    glm::vec4 aabbMaxFit = mModelMatrix * glm::vec4(aabbMax, 1.0f);
    glm::vec4 aabbSizeFit = aabbMaxFit - aabbMinFit;

    mModelMatrix =
        glm::translate(glm::vec3(-aabbSizeFit.x / 2.0f, aabbSizeFit.z / 2.0f, -aabbSizeFit.y / 2.0f)) * mModelMatrix;
    mModelMatrix = glm::rotate(glm::radians(mModelRoll), glm::vec3(1, 0, 0)) * mModelMatrix;
    mModelMatrix = glm::translate(mModelTranslate) * mModelMatrix;

    aabbMinFit = mModelMatrix * glm::vec4(aabbMin, 1.0f);
    aabbMaxFit = mModelMatrix * glm::vec4(aabbMax, 1.0f);
    aabbSizeFit = aabbMaxFit - aabbMinFit;
    const glm::vec4 aabbDiagonal1Fit = mModelMatrix * glm::vec4(aabbMin.x, aabbMin.y + aabbSize.y, aabbMin.z, 1.0f);
    const glm::vec4 aabbDiagonal2Fit = mModelMatrix * glm::vec4(aabbMin.x, aabbMin.y, aabbMin.z + aabbSize.z, 1.0f);

    mGridOffset = std::min(aabbDiagonal2Fit.y, std::min(aabbDiagonal1Fit.y, std::min(aabbMinFit.y, aabbMaxFit.y)));
    mModelShader->uniform("uGridOffset", mGridOffset);
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
    if(glData.isDirty || !mBatch || isVertexNormalIndexOverride()) {
        if(glData.isDirty && !isVertexNormalIndexOverride()) {
            // attention! do not update geometry buffers if isVertexNormalIndexOverride() is true,
            // because ExportAssistant could be modifying the geometry in a background thread
            // and the operations are not thread-safe!
            mApplication.getCurrentGeometry()->updateOpenGlBuffers();
            CI_LOG_I("Geometry buffers updated");
        }
        updateVboAndBatch();
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

    // assert(!mColorOverride.isOverriden || mColorOverride.overrideColorBuffer.size() == glData.vertexBuffer.size());

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

    if(triangleId.getDetailId() &&
       triangleId.getDetailId() >= geometry->getTriangleDetailCount(triangleId.getBaseId())) {
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

void ModelView::drawCaption(const std::string& caption, const std::string& errorCaption) {
    const float width = ImGui::GetContentRegionAvailWidth();
    const float padding = ImGui::GetStyle().WindowPadding.x;
    glm::vec2 cursorPos(10.0f, 10.0f + mApplication.getToolbar().getHeight());
    auto* drawList = ImGui::GetWindowDrawList();
    drawList->PushClipRectFullScreen();
    drawList->AddText(cursorPos, static_cast<ImColor>(ci::ColorA::hex(0x1C2A35)), caption.c_str());
    drawList->AddText(cursorPos + glm::vec2(0.0f, 16.0f), static_cast<ImColor>(ci::ColorA::hex(0xEB5757)),
                      errorCaption.c_str());
    drawList->PopClipRect();
}

}  // namespace pepr3d