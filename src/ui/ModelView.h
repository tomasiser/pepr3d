#pragma once

#include "cinder/CameraUi.h"
#include "cinder/Utilities.h"
#include "cinder/gl/gl.h"
#include "glm/glm.hpp"

#include "peprimgui.h"

namespace pepr3d {

class MainApplication;

class ModelView {
   public:
    ModelView(MainApplication& app) : mApplication(app) {}
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

   private:
    MainApplication& mApplication;
    std::pair<glm::ivec2, glm::ivec2> mViewport;
    ci::CameraPersp mCamera;
    ci::CameraUi mCameraUi;
    ci::gl::GlslProgRef mModelShader;
    bool mIsWireframeEnabled = false;
    bool mIsGridEnabled = true;
    glm::mat4 mModelMatrix;
    float mModelRoll = 0.0f;
    glm::vec3 mModelTranslate = glm::vec3(0);
    void updateModelMatrix();
};

}  // namespace pepr3d