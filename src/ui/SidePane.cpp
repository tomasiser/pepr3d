#include "SidePane.h"
#include "MainApplication.h"
#include "commands/CmdChangeColorManagerColor.h"
#include "commands/CmdReplaceColorManagerColors.h"
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

    ImGui::SetNextWindowPos(glm::ivec2(mApplication.getWindowSize().x - size.x, 49));
    ImGui::SetNextWindowSize(glm::ivec2(size.x + 1.0f, size.y - mApplication.getToolbar().getHeight() + 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, glm::vec2(12.0f));
    ImGui::Begin("##sidepane-inside", nullptr, window_flags);
    currentTool.drawToSidePane(*this);
    ImGui::End();

    ImGui::PopStyleVar(5);
    ImGui::PopStyleColor(6);
}

void SidePane::drawText(std::string text) {
    ImGui::Text(text.c_str());
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
    glm::ivec2 cursorPos = ImGui::GetCursorScreenPos();
    auto* drawList = ImGui::GetWindowDrawList();
    drawList->PushClipRectFullScreen();
    drawList->AddLine(glm::ivec2(mApplication.getWindowSize().x - mWidth, cursorPos.y),
                      glm::ivec2(mApplication.getWindowSize().x, cursorPos.y), (ImColor)ci::ColorA::hex(0xEDEDED));
    drawList->PopClipRect();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);
}

void SidePane::drawColorPalette(ColorManager& colorManager) {
    drawText("Color");

    // TODO: https://github.com/tomasiser/pepr3d/issues/41

    // int colorCount = colorManager.size();
    // if (ImGui::DragInt("##colorPaletteAmount", &colorCount, 0.05f, 1, PEPR3D_MAX_PALETTE_COLORS)) {
    //     int diff = colorManager.size() - colorCount;
    //     while (diff != 0) {
    //         if (diff > 0) {
    //             colorManager.popColor();
    //             --diff;
    //         } else {
    //             colorManager.addColor(colorManager.getColor(colorManager.size() - 1));
    //             ++diff;
    //         }
    //     }
    // }

    CommandManager<Geometry>* const commandManager = mApplication.getCommandManager();

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
            colorManager.setActiveColorIndex(i);
        }
        if(ImGui::IsItemClicked(1)) {
            ImGui::OpenPopup(colorEditPopupId.c_str());
        }
        std::string hotkeyString;
        if(i < 10) {
            HotkeyAction action = static_cast<HotkeyAction>(static_cast<std::size_t>(HotkeyAction::SelectColor1) + i);
            auto hotkey = mApplication.getHotkeys().findHotkey(action);
            if(hotkey) {
                hotkeyString = hotkey->getString();
            }
        }
        mApplication.drawTooltipOnHover("Color " + std::to_string(i + 1), hotkeyString,
                                        "Select: left click\nEdit: right click", "",
                                        glm::vec2(-18.0f + cursorPos.x, cursorPos.y), glm::vec2(1.0f, 0.0f));

        drawList->AddRect(
            cursorPos + glm::ivec2(static_cast<int>(leftCornerX), 0),
            cursorPos + glm::ivec2(static_cast<int>(leftCornerX + boxWidth + 2), static_cast<int>(boxHeight + 2)),
            (ImColor)ci::ColorA::hex(0xEDEDED));

        glm::vec4 color = colorManager.getColor(i);
        drawList->AddRectFilled(
            cursorPos + glm::ivec2(static_cast<int>(leftCornerX + 1), 1),
            cursorPos + glm::ivec2(static_cast<int>(leftCornerX + 1 + boxWidth), static_cast<int>(boxHeight + 1)),
            (ImColor)color);
        if(isSelected) {
            float brightness = 0.2126f * color.r + 0.7152f * color.g + 0.0722f * color.b;
            glm::vec4 selectColor = (brightness > 0.75f) ? ci::ColorA::hex(0x1C2A35) : ci::ColorA::hex(0xFCFCFC);
            drawList->AddRect(cursorPos + glm::ivec2(static_cast<int>(leftCornerX + 1 + 2), 1 + 2),
                              cursorPos + glm::ivec2(static_cast<int>(leftCornerX + 1 + boxWidth - 2),
                                                     static_cast<int>(boxHeight + 1 - 2)),
                              (ImColor)selectColor, 0.0f, 15, 5.0f);
        }

        ImGui::SetNextWindowPos(cursorPos + glm::ivec2(0, static_cast<int>(boxHeight) + 2));
        ImGui::SetNextWindowSize(glm::vec2(ImGui::GetContentRegionAvailWidth(), 285));
        if(ImGui::BeginPopup(colorEditPopupId.c_str())) {
            ImGui::PushItemWidth(-0.001f);  // force full width
            if(ImGui::ColorPicker3(colorPickerId.c_str(), &color[0], ImGuiColorEditFlags_NoSidePreview)) {
                assert(commandManager);
                commandManager->execute(std::make_unique<CmdChangeColorManagerColor>(i, color), true);
            }
            ImGui::PopItemWidth();
            ImGui::EndPopup();
        }

        leftCornerX += boxWidth + 1;
    }

    ImGui::SetCursorScreenPos(initialCursorPos);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + rowCount * (boxHeight + 2.f) + 10.0f);

    if(drawButton("Randomize Colors")) {
        std::random_device rd;   // Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd());  // Standard mersenne_twister_engine seeded with rd()
        std::uniform_real_distribution<> dis(0.0, 1.0);
        std::vector<glm::vec4> newColors;
        for(int i = 0; i < colorManager.size(); ++i) {
            newColors.emplace_back(dis(gen), dis(gen), dis(gen), 1);
        }
        assert(commandManager);
        commandManager->execute(std::make_unique<CmdReplaceColorManagerColors>(std::move(newColors)));
    }
}

void SidePane::drawTooltipOnHover(const std::string& label, const std::string& shortcut, const std::string& description,
                                  const std::string& disabled) {
    glm::vec2 position = ImGui::GetItemRectMin();
    position += glm::vec2(-18.0f, 0.0f);
    const glm::vec2 pivot(1.0f, 0.0f);
    mApplication.drawTooltipOnHover(label, shortcut, description, disabled, position, pivot);
}

}  // namespace pepr3d