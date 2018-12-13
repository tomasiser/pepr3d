#pragma once

#include "cinder/Utilities.h"
#include "cinder/gl/gl.h"
#include "glm/glm.hpp"

#include "peprimgui.h"

#include "ui/CameraUi.h"

namespace pepr3d {

class MainApplication;

class ModelView {
   public:
    explicit ModelView(MainApplication& app) : mApplication(app) {}
    void setup();
    void resize();
    void draw();
    void drawGeometry();

    void onMouseDown(ci::app::MouseEvent event);
    void onMouseDrag(ci::app::MouseEvent event);
    void onMouseUp(ci::app::MouseEvent event);
    void onMouseWheel(ci::app::MouseEvent event);
    void onMouseMove(ci::app::MouseEvent event);

    ci::Ray getRayFromWindowCoordinates(glm::ivec2 windowCoords) const;
    void drawTriangleHighlight(const size_t triangleIndex);

    bool isWireframeEnabled() const {
        return mIsWireframeEnabled;
    }

    void enableWireframe(bool enable = true) {
        mIsWireframeEnabled = enable;
    }

    bool isGridEnabled() const {
        return mIsGridEnabled;
    }

    void enableGrid(bool enable = true) {
        mIsGridEnabled = enable;
    }

    float getModelRoll() const {
        return mModelRoll;
    }

    void setModelRoll(float roll) {
        mModelRoll = roll;
    }

    glm::vec3 getModelTranslate() const {
        return mModelTranslate;
    }

    void setModelTranslate(glm::vec3 translate) {
        mModelTranslate = translate;
    }

    void resetCamera();

    void setColorOverride(bool val) {
        mColorOverride.isOverriden = val;
    }

    bool isColorOverride() const {
        return mColorOverride.isOverriden;
    }

    std::vector<glm::vec4>& getOverrideColorBuffer() {
        return mColorOverride.overrideColorBuffer;
    }

    void updateVboAndBatch();

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

    struct ColorOverrideData {
        bool isOverriden = false;
        std::vector<glm::vec4> overrideColorBuffer;
    } mColorOverride;

    void updateModelMatrix();
    struct Attributes {
        static const cinder::geom::Attrib COLOR_IDX = cinder::geom::Attrib::CUSTOM_0;
        static const cinder::geom::Attrib HIGHLIGHT_MASK = cinder::geom::Attrib::CUSTOM_1;
    };
};

}  // namespace pepr3d