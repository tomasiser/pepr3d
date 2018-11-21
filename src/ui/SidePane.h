#pragma once

#include "commands/CommandManager.h"
#include "geometry/ColorManager.h"
#include "peprimgui.h"

namespace pepr3d {

class MainApplication;

class SidePane {
   public:
    SidePane(MainApplication& app) : mApplication(app) {}

    float getWidth() {
        return mWidth;
    }

    void draw();

    void drawText(std::string text);
    bool drawButton(std::string label);
    void drawSeparator();
    void drawColorPalette(ColorManager& colorManager);

    template <typename Callback>
    void drawCheckbox(std::string label, bool isChecked, Callback onChanged) {
        if(ImGui::Checkbox(label.c_str(), &isChecked)) {
            onChanged(isChecked);
        }
    }

    void drawCheckbox(std::string label, bool& isChecked) {
        ImGui::Checkbox(label.c_str(), &isChecked);
    }

    void drawIntDragger(std::string label, int& value, float dragSpeed, int minValue, int maxValue,
                        std::string displayFormat, float width) {
        ImGui::PushItemWidth(width);
        ImGui::DragInt(label.c_str(), &value, dragSpeed, minValue, maxValue, displayFormat.c_str());
        ImGui::PopItemWidth();
    }

    void drawVec3Dragger(std::string label, glm::vec3& value, float dragSpeed, float minValue, float maxValue,
                         std::string displayFormat, float width) {
        ImGui::PushItemWidth(width);
        ImGui::DragFloat3(label.c_str(), &value[0], dragSpeed, minValue, maxValue, displayFormat.c_str());
        ImGui::PopItemWidth();
    }

    void setCommandManager(CommandManager<class Geometry>* commandManager) {
        mCommandManager = commandManager;
    }

   private:
    MainApplication& mApplication;
    CommandManager<class Geometry>* mCommandManager = nullptr;
    float mWidth = 235.0f;
};

}  // namespace pepr3d