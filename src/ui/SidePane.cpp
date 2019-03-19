#include "SidePane.h"
#include "MainApplication.h"
#include "commands/CmdColorManager.h"
#include "geometry/Geometry.h"
#include "tools/Tool.h"

namespace pepr3d {

void SidePane::draw() {
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoScrollbar;
    window_flags |= ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoNav;

    glm::ivec2 size(mWidth, mApplication.getWindowSize().y);

    ImGui::SetNextWindowPos(glm::ivec2(mApplication.getWindowSize().x - size.x, 0));
    ImGui::SetNextWindowSize(glm::ivec2(size.x + 1.0f, mApplication.getToolbar().getHeight()));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ci::ColorA::hex(0xFFFFFF));
    ImGui::PushStyleColor(ImGuiCol_Border, ci::ColorA::hex(0xEDEDED));
    ImGui::PushStyleColor(ImGuiCol_Text, ci::ColorA::hex(0x1C2A35));
    ImGui::PushStyleColor(ImGuiCol_Button, ci::ColorA::zero());
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ci::ColorA::hex(0xCFD5DA));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ci::ColorA::hex(0xA3B2BF));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, glm::vec2(0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, glm::ivec2(8, 10));

    ImGui::Begin("##sidepane-header", nullptr, window_flags);

    auto& currentTool = **mApplication.getCurrentToolIterator();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    glm::ivec2 cursorPos = ImGui::GetCursorScreenPos();
    drawList->AddLine(cursorPos + glm::ivec2(0, 49), cursorPos + glm::ivec2(size.x, 49),
                      ImColor(ci::ColorA::hex(0xEDEDED)));

    ImGui::SetCursorPos(glm::ivec2(12, 13));
    ImGui::PushFont(mApplication.getFontStorage().getRegularIconFont());
    ImGui::Text(currentTool.getIcon().c_str());
    ImGui::PopFont();
    ImGui::SetCursorPos(glm::ivec2(50, 15));
    ImGui::Text(currentTool.getName().c_str());

    ImGui::End();

    window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoNav;

    ImGui::SetNextWindowPos(glm::ivec2(mApplication.getWindowSize().x - size.x, 49));
    ImGui::SetNextWindowSize(glm::ivec2(size.x + 1.0f, size.y - mApplication.getToolbar().getHeight() + 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, glm::vec2(12.0f));
    ImGui::Begin("##sidepane-inside", nullptr, window_flags);
    currentTool.drawToSidePane(*this);
    ImGui::End();

    ImGui::PopStyleVar(5);
    ImGui::PopStyleColor(6);
}

void SidePane::resize() {
    float windowWidth = static_cast<float>(mApplication.getWindowSize().x);
    mWidth = std::max<float>(200.0f, std::min<float>(windowWidth - 250.0f, mWidth));
}

void SidePane::drawText(std::string text) {
    ImGui::TextWrapped(text.c_str());
}

bool SidePane::drawButton(std::string label) {
    bool ret = ImGui::Button(label.c_str(), glm::ivec2(ImGui::GetContentRegionAvailWidth(), 33));
    ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                        (ImColor)ci::ColorA::hex(0xEDEDED));
    return ret;
}

bool SidePane::drawColoredButton(std::string label, const ci::ColorA color, const float borderThickness) {
    ImGui::PushStyleColor(ImGuiCol_Border, color);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, borderThickness);
    bool ret = ImGui::Button(label.c_str(), glm::ivec2(ImGui::GetContentRegionAvailWidth(), 33));
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    return ret;
}

void SidePane::drawSeparator() {
    const float width = ImGui::GetContentRegionAvailWidth();
    const float padding = ImGui::GetStyle().WindowPadding.x;
    glm::ivec2 cursorPos = ImGui::GetCursorScreenPos();
    auto* drawList = ImGui::GetWindowDrawList();
    drawList->PushClipRectFullScreen();
    drawList->AddLine(glm::vec2(mApplication.getWindowSize().x - mWidth, cursorPos.y),
                      glm::vec2(mApplication.getWindowSize().x - mWidth + width + 2.0f * padding, cursorPos.y),
                      (ImColor)ci::ColorA::hex(0xEDEDED));
    drawList->PopClipRect();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);
}

void SidePane::drawColorPalette(const std::string& label, bool isEditable) {
    Geometry* const geometry = mApplication.getCurrentGeometry();
    if(geometry == nullptr) {
        return;
    }

    ColorManager& colorManager = geometry->getColorManager();
    CommandManager<Geometry>* const commandManager = mApplication.getCommandManager();
    assert(commandManager != nullptr);

    // Header:
    if(!label.empty()) {
        drawText(label.c_str());
    }

    // Add button and drag&drop box for deleting colors:
    if(isEditable) {
        drawColorPaletteAddRemoveButtons(colorManager, *commandManager);
    }

    // Color boxes (selectable or editable):
    drawColorPaletteColorBoxes(colorManager, *commandManager, isEditable);

    // Reset to defaults button:
    if(isEditable) {
        if(drawButton("Reset all colors to default")) {
            commandManager->execute(std::make_unique<CmdColorManagerResetColors>());
        }
        drawTooltipOnHover("Remove all colors and replace them with defaults.", "", "", "");
    }
}

void SidePane::drawColorPaletteAddRemoveButtons(ColorManager& colorManager, CommandManager<Geometry>& commandManager) {
    ImGui::TextWrapped("Edit palette by left clicking on the colors. Also try drag & dropping!");
    if(drawButton("Add color")) {
        if(colorManager.size() >= PEPR3D_MAX_PALETTE_COLORS) {
            mApplication.pushDialog(
                Dialog(DialogType::Error, "Could not add color", "The maximum number of colors has been reached."));
        } else {
            commandManager.execute(std::make_unique<CmdColorManagerAddColor>());
        }
    }
    drawTooltipOnHover("Add a new color to the palette.", "", "", "");

    const glm::vec2 size(ImGui::GetContentRegionAvailWidth(), 33.0f);
    ImGui::InvisibleButton("colorPaletteRemoveButton", size);
    drawTooltipOnHover("Drag & drop color here to delete it entirely.", "",
                       "The removed color on the model will be replaced by the 1st color in the palette.", "");
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    drawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), (ImColor)ci::ColorA::hex(0xEDEDED));

    bool isActiveDragDropTarget = false;

    if(ImGui::BeginDragDropTarget()) {
        if(const ImGuiPayload* payload =
               ImGui::AcceptDragDropPayload("colorPaletteDragDrop", ImGuiDragDropFlags_AcceptPeekOnly)) {
            assert(payload->DataSize == sizeof(size_t));
            isActiveDragDropTarget = true;
            if(payload->IsDelivery()) {
                if(colorManager.size() > 1) {
                    commandManager.execute(
                        std::make_unique<CmdColorManagerRemoveColor>(*static_cast<const size_t*>(payload->Data)));
                } else {
                    mApplication.pushDialog(Dialog(DialogType::Error, "Could not remove color",
                                                   "There has to be at least 1 color in the palette."));
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    const std::string text = isActiveDragDropTarget ? "Drop to delete!" : "Drag color here to delete";
    const glm::vec2 textSize = ImGui::CalcTextSize(text.c_str());
    const glm::vec2 textPosition = static_cast<glm::vec2>(ImGui::GetItemRectMin()) + (size - textSize) / 2.0f;
    glm::vec4 textColor = ci::ColorA::hex(0xEB5757);
    if(isActiveDragDropTarget) {
        drawList->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), (ImColor)ci::ColorA::hex(0xEB5757));
        textColor = ci::ColorA::hex(0xFFFFFF);
    }
    drawList->AddText(textPosition, (ImColor)textColor, text.c_str());
}

void SidePane::drawColorPaletteColorBoxes(ColorManager& colorManager, CommandManager<Geometry>& commandManager,
                                          bool isEditable) {
    glm::ivec2 cursorPos = ImGui::GetCursorScreenPos();
    glm::ivec2 initialCursorPos = ImGui::GetCursorScreenPos();
    ImDrawList* const drawList = ImGui::GetWindowDrawList();

    const size_t boxesPerRow = std::min<size_t>(colorManager.size(), 4);
    const float boxHeight = 38.0f;  // without border
    const float boxWidth =          // without border
        (ImGui::GetContentRegionAvailWidth() - (static_cast<float>(boxesPerRow) + 1.0f)) /
        static_cast<float>(boxesPerRow);  // subtract 1px per each separator and boundary and divide by number of boxes
    size_t rowCount = 1;
    float leftCornerX = 0;
    for(size_t i = 0; i < colorManager.size(); ++i) {
        if(i != 0 && i % boxesPerRow == 0) {
            cursorPos += glm::ivec2(0, static_cast<int>(boxHeight) + 1);
            ++rowCount;
            leftCornerX = 0;
        }

        const bool isSelected = i == colorManager.getActiveColorIndex();
        const std::string colorEditPopupId = std::string("##colorPaletteEditPopup") + std::to_string(i);
        const std::string colorPickerId = std::string("##colorPalettePicker") + std::to_string(i);

        ImGui::SetCursorScreenPos(cursorPos + glm::ivec2(static_cast<int>(leftCornerX + 1), 1));
        if(ImGui::InvisibleButton((std::string("colorPaletteButton") + std::to_string(i)).c_str(),
                                  glm::vec2(boxWidth, boxHeight))) {
            if(isEditable) {
                ImGui::OpenPopup(colorEditPopupId.c_str());
            } else {
                colorManager.setActiveColorIndex(i);
            }
        }
        const bool isHovered = ImGui::IsItemHovered();
        const bool isHeld = ImGui::IsItemActive();
        std::string hotkeyString;
        if(!isEditable && i < 10) {
            HotkeyAction action = static_cast<HotkeyAction>(static_cast<std::size_t>(HotkeyAction::SelectColor1) + i);
            auto hotkey = mApplication.getHotkeys().findHotkey(action);
            if(hotkey) {
                hotkeyString = hotkey->getString();
            }
        }
        mApplication.drawTooltipOnHover(
            "Color " + std::to_string(i + 1), hotkeyString,
            isEditable ? "Click to change color\nDrag & drop to reorder colors\nCtrl + Drag & drop to swap colors"
                       : "Click to select",
            "", glm::vec2(-18.0f + cursorPos.x, cursorPos.y), glm::vec2(1.0f, 0.0f));

        bool isActiveDragDropTarget = false;
        if(isEditable) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, glm::vec2(0.0f));
            if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                ImGui::SetDragDropPayload("colorPaletteDragDrop", &i, sizeof(size_t));
                ImDrawList* const tooltipDrawList = ImGui::GetWindowDrawList();
                const glm::vec2 tooltipCursorPos = ImGui::GetCursorScreenPos();
                const glm::vec2 tooltipSize(boxWidth, boxHeight);
                tooltipDrawList->AddRectFilled(tooltipCursorPos, tooltipCursorPos + tooltipSize,
                                               (ImColor)colorManager.getColor(i));
                ImGui::SetCursorScreenPos(tooltipCursorPos + tooltipSize);
                ImGui::EndDragDropSource();
            }
            ImGui::PopStyleVar(1);
            if(ImGui::BeginDragDropTarget()) {
                if(const ImGuiPayload* payload =
                       ImGui::AcceptDragDropPayload("colorPaletteDragDrop", ImGuiDragDropFlags_AcceptPeekOnly)) {
                    assert(payload->DataSize == sizeof(size_t));
                    isActiveDragDropTarget = true;
                    if(payload->IsDelivery()) {
                        size_t payloadi = *static_cast<const size_t*>(payload->Data);
                        if(ImGui::GetIO().KeyCtrl) {
                            // swap colors if Ctrl is held:
                            commandManager.execute(std::make_unique<CmdColorManagerSwapColors>(i, payloadi));
                        } else {
                            // reorder colors:
                            commandManager.execute(std::make_unique<CmdColorManagerReorderColors>(i, payloadi));
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }
        }

        drawList->AddRect(
            cursorPos + glm::ivec2(static_cast<int>(leftCornerX), 0),
            cursorPos + glm::ivec2(static_cast<int>(leftCornerX + boxWidth + 2), static_cast<int>(boxHeight + 2)),
            (ImColor)ci::ColorA::hex(0xEDEDED));

        glm::vec4 color = colorManager.getColor(i);
        const float boxAlpha = (isHovered || isHeld) ? (isHeld ? 0.8f : 0.9f) : 1.0f;
        const glm::vec4 boxColor(color.r, color.g, color.b, boxAlpha);
        drawList->AddRectFilled(
            cursorPos + glm::ivec2(static_cast<int>(leftCornerX + 1), 1),
            cursorPos + glm::ivec2(static_cast<int>(leftCornerX + 1 + boxWidth), static_cast<int>(boxHeight + 1)),
            (ImColor)boxColor);
        if(isActiveDragDropTarget || (!isEditable && isSelected)) {
            const float brightness = 0.2126f * color.r + 0.7152f * color.g + 0.0722f * color.b;
            const glm::vec4 selectColor = (brightness > 0.75f) ? ci::ColorA::hex(0x1C2A35) : ci::ColorA::hex(0xFCFCFC);
            const float thickness = 5.0f;
            const float offset = (thickness - 1.0f) / 2.0f;
            drawList->AddRect(cursorPos + glm::ivec2(static_cast<int>(leftCornerX + 1 + offset), 1 + offset),
                              cursorPos + glm::ivec2(static_cast<int>(leftCornerX + 1 + boxWidth - offset),
                                                     static_cast<int>(boxHeight + 1 - offset)),
                              (ImColor)selectColor, 0.0f, 15, thickness);
        }
        if(isEditable) {
            float approximateColorPickerHeight = 300.0f;
            const glm::vec2 colorPickerSize(ImGui::GetContentRegionAvailWidth(), 0.0f);
            glm::vec2 colorPickerPos(ImGui::GetIO().DisplaySize.x - mWidth - colorPickerSize.x + 1.0f,
                                     cursorPos.y + boxHeight - approximateColorPickerHeight / 2.0f);
            if(colorPickerPos.y + approximateColorPickerHeight > ImGui::GetIO().DisplaySize.y) {
                colorPickerPos.y = ImGui::GetIO().DisplaySize.y - approximateColorPickerHeight - 1.0f;
            } else if(colorPickerPos.y < mApplication.getToolbar().getHeight() - 1.0f) {
                colorPickerPos.y = mApplication.getToolbar().getHeight() - 1.0f;
            }
            ImGui::SetNextWindowPos(colorPickerPos);
            ImGui::SetNextWindowSize(colorPickerSize);
            if(ImGui::BeginPopup(colorEditPopupId.c_str())) {
                ImGui::PushItemWidth(-0.001f);  // force full width
                if(ImGui::ColorPicker3(colorPickerId.c_str(), &color[0], ImGuiColorEditFlags_NoSidePreview)) {
                    commandManager.execute(std::make_unique<CmdColorManagerChangeColor>(i, color), true);
                }
                ImGui::PopItemWidth();
                ImGui::EndPopup();
            }
        }

        leftCornerX += boxWidth + 1;
    }

    // Move cursor under the boxes:
    ImGui::SetCursorScreenPos(initialCursorPos);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + rowCount * (boxHeight + 2.f) + 10.0f);
}

void SidePane::drawTooltipOnHover(const std::string& label, const std::string& shortcut, const std::string& description,
                                  const std::string& disabled) {
    glm::vec2 position = ImGui::GetItemRectMin();
    position += glm::vec2(-18.0f, 0.0f);
    const glm::vec2 pivot(1.0f, 0.0f);
    mApplication.drawTooltipOnHover(label, shortcut, description, disabled, position, pivot);
}

void SidePane::Category::drawHeader(SidePane& sidePane) {
    glm::vec2 cursorPos = ImGui::GetCursorScreenPos();
    cursorPos += glm::vec2(-11.0f, -11.0f);
    ImGui::SetCursorScreenPos(cursorPos);

    const glm::vec2 size(sidePane.mWidth, 40.0f);

    if(ImGui::InvisibleButton("categoryHeader", size)) {
        mIsOpen = !mIsOpen;
    }
    const bool isHovered = ImGui::IsItemHovered();
    const bool isHeld = ImGui::IsItemActive();

    auto* const drawList = ImGui::GetWindowDrawList();
    drawList->PushClipRectFullScreen();
    if(isHovered || isHeld) {
        drawList->AddRectFilled(cursorPos, cursorPos + size,
                                isHeld ? (ImColor)ci::ColorA::hex(0xA3B2BF) : (ImColor)ci::ColorA::hex(0xCFD5DA));
    }
    drawList->PopClipRect();

    ImGui::PushFont(sidePane.mApplication.getFontStorage().getRegularIconFont());
    ImGui::SetCursorScreenPos(cursorPos + glm::vec2(12.0f, 9.0f));
    ImGui::Text(mIsOpen ? ICON_MD_KEYBOARD_ARROW_DOWN : ICON_MD_KEYBOARD_ARROW_RIGHT);
    ImGui::PopFont();
    ImGui::SetCursorScreenPos(cursorPos + glm::vec2(50.0f, 11.0f));
    ImGui::Text(mCaption.c_str());
}

}  // namespace pepr3d