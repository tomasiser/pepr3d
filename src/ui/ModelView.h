#pragma once

#include "cinder/Utilities.h"
#include "cinder/gl/gl.h"
#include "glm/glm.hpp"

#include "peprimgui.h"

#include "ui/CameraUi.h"

#include <chrono>
#include "geometry/TrianglePrimitive.h"

namespace pepr3d {

class MainApplication;

/// The main part of the user interface, shows the geometry to the user
class ModelView {
   public:
    explicit ModelView(MainApplication& app) : mApplication(app) {}
    std::vector<glm::vec3> debugTriangles;
    std::vector<uint32_t> debugIndices;
    std::vector<glm::vec4> debugColors;
    std::vector<glm::vec3> debugNormals;

    /// Setups the camera and shaders. Call only once!
    void setup();

    /// Called when the ModelView should recalculate its size.
    void resize();

    /// Draws the ModelView, both ImGui and the Geometry.
    void draw();

    /// Draws the Geometry.
    void drawGeometry();

    /// On mouse-down event over the ModelView area.
    void onMouseDown(ci::app::MouseEvent event);

    /// On mouse-drag event over the ModelView area.
    void onMouseDrag(ci::app::MouseEvent event);

    /// On mouse-up event over the ModelView area.
    void onMouseUp(ci::app::MouseEvent event);

    /// On mouse-wheel event over the ModelView area.
    void onMouseWheel(ci::app::MouseEvent event);

    /// On mouse-move event over the ModelView area.
    void onMouseMove(ci::app::MouseEvent event);

    /// Returns a 3D ray in the Geometry scene computed from window coordinates.
    ci::Ray getRayFromWindowCoordinates(glm::ivec2 windowCoords) const;

    /// Draws a highlight (3 lines with a contrasting color) of a specified DetailedTriangleId.
    void drawTriangleHighlight(const DetailedTriangleId triangleId);

    /// Draws a highlight (3 lines with a contrasting color) of a specified triangle ID.
    void drawTriangleHighlight(const size_t triangleIdx) {
        drawTriangleHighlight(DetailedTriangleId(triangleIdx));
    }

    /// Draws a 3D line.
    void drawLine(const glm::vec3& from, const glm::vec3& to, const ci::Color& color = ci::Color::white(),
                  float width = 1.0f);

    /// Draws a text label (caption) to the top left corner.
    /// `caption` is black, `errorCaption` is red and below `caption`
    void drawCaption(const std::string& caption, const std::string& errorCaption);

    /// Returns true if wireframe of Geometry is rendered.
    bool isWireframeEnabled() const {
        return mIsWireframeEnabled;
    }

    /// Sets whether the wireframe of Geometry should be rendered.
    void enableWireframe(bool enable = true) {
        mIsWireframeEnabled = enable;
    }

    /// Returns true if the grid below the Geometry is rendered.
    bool isGridEnabled() const {
        return mIsGridEnabled;
    }

    /// Sets whether the grid below the Geometry should be rendered.
    void enableGrid(bool enable = true) {
        mIsGridEnabled = enable;
    }

    /// Returns the current roll (rotation in the axis not controlled by CameraUi) of the Geometry object.
    float getModelRoll() const {
        return mModelRoll;
    }

    /// Sets the current roll (rotation in the axis not controlled by CameraUi) of the Geometry object.
    void setModelRoll(float roll) {
        mModelRoll = roll;
    }

    /// Gets the 3D translation of the Geometry object.
    glm::vec3 getModelTranslate() const {
        return mModelTranslate;
    }

    /// Sets the 3D translation of the Geometry object.
    void setModelTranslate(glm::vec3 translate) {
        mModelTranslate = translate;
    }

    /// Resets the camera to the default position.
    void resetCamera();

    /// Resets the camera and sets zoom to be field-of-view based (so on zoom, the camera position does not change, only
    /// FOV).
    void setFovZoomEnabled(bool enabled) {
        resetCamera();
        mCameraUi.setFovZoomEnabled(enabled);
    }

    /// Returns true if the camera zoom is field-of-view based (so on zoom, the camera position does not change, only
    /// FOV).
    bool isFovZoomEnabled() const {
        return mCameraUi.isFovZoomEnabled();
    }

    /// Enables or disables color buffer override.
    void setColorOverride(bool val) {
        mColorOverride.isOverriden = val;
    }

    /// Returns true if the color buffer is overridden.
    bool isColorOverride() const {
        return mColorOverride.isOverriden;
    }

    /// Returns a reference to the override color buffer, so you can read it or write to it.
    std::vector<glm::vec4>& getOverrideColorBuffer() {
        return mColorOverride.overrideColorBuffer;
    }

    /// Enables or disables vertex, normal, and index buffer override (all at once)
    void setVertexNormalIndexOverride(bool val) {
        mVertexNormalIndexOverride.isOverriden = val;
        mBatch = nullptr;  // reset batch to force update
    }

    /// Returns true if the vertex, normal, and index buffers are overriden (all at once)
    bool isVertexNormalIndexOverride() const {
        return mVertexNormalIndexOverride.isOverriden;
    }

    /// Returns a reference to the override vertex buffer, so you can read it or write to it.
    std::vector<glm::vec3>& getOverrideVertexBuffer() {
        return mVertexNormalIndexOverride.overrideVertexBuffer;
    }

    /// Returns a reference to the override normal buffer, so you can read it or write to it.
    std::vector<glm::vec3>& getOverrideNormalBuffer() {
        return mVertexNormalIndexOverride.overrideNormalBuffer;
    }

    /// Returns a reference to the override index buffer, so you can read it or write to it.
    std::vector<uint32_t>& getOverrideIndexBuffer() {
        return mVertexNormalIndexOverride.overrideIndexBuffer;
    }

    /// Returns the minimum (first) and maximum (second) height that is rendered in the ModelView.
    glm::vec2 getPreviewMinMaxHeight() const {
        return mPreviewMinMaxHeight;
    }

    /// Sets the minimum (first) and maximum (second) height that is rendered in the ModelView.
    void setPreviewMinMaxHeight(glm::vec2 minMax) {
        mPreviewMinMaxHeight = minMax;
        mModelShader->uniform("uPreviewMinMaxHeight", mPreviewMinMaxHeight);
    }

    /// Returns the maximum size (in the X, Y, Z axes) of the current Geometry object.
    float getMaxSize() const {
        return mMaxSize;
    }

    /// Called by MainAplication when new geometry is loaded
    void onNewGeometryLoaded();

    glm::mat4 getModelMatrix() const {
        return mModelMatrix;
    }

    const ci::CameraPersp& getCamera() const {
        return mCamera;
    }

   private:
    MainApplication& mApplication;
    ci::gl::VboMeshRef mVboMesh;
    ci::gl::BatchRef mBatch;

    std::pair<glm::ivec2, glm::ivec2> mViewport;
    ci::CameraPersp mCamera;
    pepr3d::CameraUi mCameraUi;
    ci::gl::GlslProgRef mModelShader;
    bool mIsWireframeEnabled = false;
    bool mIsGridEnabled = true;
    float mGridOffset = 0.0f;
    glm::mat4 mModelMatrix;
    float mModelRoll = 0.0f;
    glm::vec3 mModelTranslate = glm::vec3(0);
    float mMaxSize = 1.f;
    glm::vec2 mPreviewMinMaxHeight = glm::vec2(0.0f, 1.0f);

    /// Color buffer override
    struct ColorOverrideData {
        bool isOverriden = false;
        std::vector<glm::vec4> overrideColorBuffer;
    } mColorOverride;

    /// Vertex, normal, and index buffer override
    struct VertexNormalIndexOverrideData {
        bool isOverriden = false;
        std::vector<glm::vec3> overrideVertexBuffer;
        std::vector<glm::vec3> overrideNormalBuffer;
        std::vector<uint32_t> overrideIndexBuffer;
    } mVertexNormalIndexOverride;

    /// Recalculates the model matrix of the current Geometry object.
    /// The model matrix ensures that the object's maximum displayed size is 1.0 and it is centered above the grid,
    /// touching it on the bottom.
    void updateModelMatrix();

    /// Recalculates the OpenGL vertex buffer object and the Cinder batch.
    void updateVboAndBatch();

    /// Custom OpenGL attributes
    struct Attributes {
        static const cinder::geom::Attrib COLOR_IDX = cinder::geom::Attrib::CUSTOM_0;
        static const cinder::geom::Attrib HIGHLIGHT_MASK = cinder::geom::Attrib::CUSTOM_1;
    };
};

}  // namespace pepr3d