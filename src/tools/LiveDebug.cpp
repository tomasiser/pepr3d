#include "tools/LiveDebug.h"
#include "ui/MainApplication.h"

namespace pepr3d {

void LiveDebug::drawToSidePane(SidePane& sidePane) {
    ImGui::BeginChild("##sidepane-livedebug");

    ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());
    if (sidePane.drawButton("Toggle ImGui demo window")) {
        mApplication.showDemoWindow(!mApplication.isDemoWindowShown());
    }
    ImGui::PopItemWidth();

    sidePane.drawSeparator();
    
    static int addedValue = 1;
    ImGui::Text("Current value: %i", mIntegerState.mInnerValue);
    if(mIntegerManager.canUndo()) {
        if(ImGui::Button("Undo")) {
            mIntegerManager.undo();
        }
    }
    if(mIntegerManager.canRedo()) {
        if(ImGui::Button("Redo")) {
            mIntegerManager.redo();
        }
    }
    ImGui::SliderInt("##addedvalue", &addedValue, 1, 10);
    ImGui::SameLine();
    if(ImGui::Button("Add")) {
        mIntegerManager.execute(std::make_unique<AddValueCommand>(addedValue));
    }

    ImGui::EndChild();
}

}