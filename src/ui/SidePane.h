#pragma once

#include "commands/CommandManager.h"
#include "geometry/ColorManager.h"
#include "geometry/Geometry.h"
#include "peprimgui.h"

namespace pepr3d {

class MainApplication;

/// Right side of the user interface that displays options of the currently selected tool
class SidePane {
   public:
    explicit SidePane(MainApplication& app) : mApplication(app) {}

    /// Returns the width.
    float getWidth() const {
        return mWidth;
    }

    /// Sets the width.
    void setWidth(float width) {
        mWidth = width;
    }

    /// Draws the pane using ImGui.
    void draw();

    /// Called when the SidePane should recalculate its size.
    void resize();

    /// Draws a (multi-line) text label.
    void drawText(std::string text);

    /// Draws a button with a maximum available width.
    bool drawButton(std::string label);

    /// Draws a button with a colored highlight.
    bool drawColoredButton(std::string label, const ci::ColorA color, const float borderThickness = 3.0f);

    /// Draws a horizontal separator.
    void drawSeparator();

    /// Draws a color palette of the current Geometry and its ColorManager.
    /// If isEditable is true, a full editable user interface will be drawn.
    void drawColorPalette(const std::string& label = "Color Palette", bool isEditable = false);

    /// Draws a checkbox with a callback called when the value is changed.
    template <typename Callback>
    void drawCheckbox(std::string label, bool isChecked, Callback onChanged) {
        if(ImGui::Checkbox(label.c_str(), &isChecked)) {
            onChanged(isChecked);
        }
    }

    /// Draws a checkbox with a read-write reference to the current value.
    void drawCheckbox(std::string label, bool& isChecked) {
        ImGui::Checkbox(label.c_str(), &isChecked);
    }

    /// Draws an ImGui dragger (slider) for an integer value.
    bool drawIntDragger(std::string label, int& value, float dragSpeed, int minValue, int maxValue,
                        std::string displayFormat, float width) {
        ImGui::PushItemWidth(width);
        const bool hasChanged =
            ImGui::DragInt(label.c_str(), &value, dragSpeed, minValue, maxValue, displayFormat.c_str());
        ImGui::PopItemWidth();
        return hasChanged;
    }

    /// Draws an ImGui dragger (slider) for an glm::vec3 value.
    bool drawVec3Dragger(std::string label, glm::vec3& value, float dragSpeed, float minValue, float maxValue,
                         std::string displayFormat, float width) {
        ImGui::PushItemWidth(width);
        const bool hasChanged =
            ImGui::DragFloat3(label.c_str(), &value[0], dragSpeed, minValue, maxValue, displayFormat.c_str());
        ImGui::PopItemWidth();
        return hasChanged;
    }

    /// Draws an ImGui dragger (slider) for an float value.
    bool drawFloatDragger(std::string label, float& value, float dragSpeed, float minValue, float maxValue,
                          std::string displayFormat, float width) {
        ImGui::PushItemWidth(width);
        const bool hasChanged =
            ImGui::DragFloat(label.c_str(), &value, dragSpeed, minValue, maxValue, displayFormat.c_str());
        ImGui::PopItemWidth();
        return hasChanged;
    }

    /// Draws a tooltip if the previous ImGui element is hovered.
    /// The tooltip is drawn on the left of the SidePane.
    void drawTooltipOnHover(const std::string& label, const std::string& shortcut = "",
                            const std::string& description = "", const std::string& disabled = "");

    /// A category of a SidePane that can be opened and closed by a user
    class Category {
        std::string mCaption;
        bool mIsOpen;

        /// Draws the clickable header of the Category.
        void drawHeader(SidePane& sidePane);

       public:
        explicit Category(const std::string& caption, bool isOpen = false) : mCaption(caption), mIsOpen(isOpen) {}

        /// Returns true if the Category is open.
        bool isOpen() const {
            return mIsOpen;
        }

        /// Draws the header and the inside (using the provided function) is the Category is open.
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

    /// Draws add & remove buttons of an editable color palette.
    void drawColorPaletteAddRemoveButtons(ColorManager& colorManager, CommandManager<Geometry>& commandManager);

    /// Draws the color boxes of a color palette.
    void drawColorPaletteColorBoxes(ColorManager& colorManager, CommandManager<Geometry>& commandManager,
                                    bool isEditable);
};

}  // namespace pepr3d