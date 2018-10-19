#pragma once

#include "cinder/CameraUi.h"
#include "cinder/gl/gl.h"
#include "glm/glm.hpp"

#include "CinderImGui.h"

namespace pepr3d {

class MainApplication;

class ModelView {
   public:
    ModelView(MainApplication& app) : mApplication(app) {}
    void setup();
    void resize();
    void draw();

   private:
    MainApplication& mApplication;
    std::pair<glm::ivec2, glm::ivec2> mViewport;
    ci::CameraPersp mCamera;
    ci::CameraUi mCameraUi;
};

}  // namespace pepr3d