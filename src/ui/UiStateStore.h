#pragma once

#include "cinder/Arcball.h"
#include "cinder/CameraUi.h"
#include "commands/ExampleCommand.h"
#include "geometry/Geometry.h"
#include "glm/glm.hpp"

namespace pepr3d {

struct MainWindowState {
    glm::vec2 size;
    bool showDemoWindow = false;
    ImFont* defaultFont = nullptr;
    ImFont* iconFont = nullptr;
};

struct ToolbarState {
    std::size_t selectedButtonIndex = 3;
    float height = 50.0f;
};

struct SidePaneState {
    float width = 235.0f;
};

struct ModelViewState {
    std::pair<glm::ivec2, glm::ivec2> viewport;
    ci::CameraPersp camera;
    ci::CameraUi cameraUi;
};

struct UiStateStore {
    MainWindowState mainWindow;
    ToolbarState toolbar;
    SidePaneState sidePane;
    ModelViewState modelView;

    IntegerState integerState;
    CommandManager<IntegerState> integerManager;

    Geometry geometryData;

    UiStateStore() : integerManager(integerState) {}
};

}  // namespace pepr3d