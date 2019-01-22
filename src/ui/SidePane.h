#pragma once

#include "commands/CommandManager.h"
#include "geometry/ColorManager.h"
#include "geometry/Geometry.h"
#include "peprimgui.h"

namespace pepr3d {

class MainApplication;

class SidePane {
   public:
    explicit SidePane(MainApplication& app) : mApplication(app) {}

    float getWidth() const {
        return mWidth;
    }

    void setWidth(float width) {
        mWidth = width;
    }

    void draw();
    void resize();

    void drawText(std::string text);
    bool drawButton(std::string label);
    bool drawColoredButton(std::string label, const ci::ColorA color, const float borderThickness = 3.0f);
    void drawSeparator();
    void drawColorPalette(const std::string& label = "Color Palette", bool isEditable = false);

    template <typename Callback>
    void drawCheckbox(std::string label, bool isChecked, Callback onChanged) {
        if(ImGui::Checkbox(label.c_str(), &isChecked)) {
            onChanged(isChecked);
        }
    }

    void drawCheckbox(std::string label, bool& isChecked) {
        ImGui::Checkbox(label.c_str(), &isChecked);
    }

    bool drawIntDragger(std::string label, int& value, float dragSpeed, int minValue, int maxValue,
                        std::string displayFormat, float width) {
        ImGui::PushItemWidth(width);
        const bool hasChanged =
            ImGui::DragInt(label.c_str(), &value, dragSpeed, minValue, maxValue, displayFormat.c_str());
        ImGui::PopItemWidth();
        return hasChanged;
    }

    bool drawVec3Dragger(std::string label, glm::vec3& value, float dragSpeed, float minValue, float maxValue,
                         std::string displayFormat, float width) {
        ImGui::PushItemWidth(width);
        const bool hasChanged =
            ImGui::DragFloat3(label.c_str(), &value[0], dragSpeed, minValue, maxValue, displayFormat.c_str());
        ImGui::PopItemWidth();
        return hasChanged;
    }

    bool drawFloatDragger(std::string label, float& value, float dragSpeed, float minValue, float maxValue,
                          std::string displayFormat, float width) {
        ImGui::PushItemWidth(width);
        const bool hasChanged =
            ImGui::DragFloat(label.c_str(), &value, dragSpeed, minValue, maxValue, displayFormat.c_str());
        ImGui::PopItemWidth();
        return hasChanged;
    }

    void drawTooltipOnHover(const std::string& label, const std::string& shortcut = "",
                            const std::string& description = "", const std::string& disabled = "");

    class Category {
        std::string mCaption;
        bool mIsOpen;

        void drawHeader(SidePane& sidePane);

       public:
        explicit Category(const std::string& caption, bool isOpen = false) : mCaption(caption), mIsOpen(isOpen) {}

        bool isOpen() const {
            return mIsOpen;
        }

        template <typename DrawInsideFunction>
        void draw(SidePane& sidePane, DrawInsideFunction drawInside) {
            ImGui::PushID(mCaption.c_str());
            drawHeader(sidePane);
            if(mIsOpen) {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);
                drawInside();
            }
            sidePane.drawSeparator();
            ImGui::PopID();
        }
    };

   private:
    MainApplication& mApplication;
    float mWidth = 250.0f;

    void drawColorPaletteAddRemoveButtons(ColorManager& colorManager, CommandManager<Geometry>& commandManager);
    void drawColorPaletteColorBoxes(ColorManager& colorManager, CommandManager<Geometry>& commandManager,
                                    bool isEditable);
};

}  // namespace pepr3d