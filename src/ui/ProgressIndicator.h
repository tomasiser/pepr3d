#pragma once

#include <memory>

#include "IconsMaterialDesign.h"
#include "peprimgui.h"

#include "imgui_internal.h"  // must be included after peprimgui.h! beware of clang-format, keep the empty line above!

#include "geometry/Geometry.h"
#include "geometry/GeometryProgress.h"

namespace pepr3d {

class MainApplication;

/// User interface for showing a current progress, contains a spinner and progress bars
class ProgressIndicator {
   public:
    void setGeometryInProgress(std::shared_ptr<Geometry> geometry) {
        mGeometry = geometry;
    }

    void draw();

   private:
    std::shared_ptr<Geometry> mGeometry;
    void drawSpinner(const char* label);
    void drawStatus(const std::string& label, float progress, bool isIndeterminate);
};

}  // namespace pepr3d
