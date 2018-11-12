#include "tools/DisplayOptions.h"
#include <random>
#include "ui/MainApplication.h"

namespace pepr3d {

void DisplayOptions::drawToSidePane(SidePane& sidePane) {
    sidePane.drawCheckbox("Show wireframe", mApplication.getModelView().isWireframeEnabled(),
                          [&](bool isChecked) { mApplication.getModelView().enableWireframe(isChecked); });
}

}  // namespace pepr3d