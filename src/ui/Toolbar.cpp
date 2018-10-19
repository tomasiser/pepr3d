#include "Toolbar.h"
#include "MainApplication.h"

namespace pepr3d {

void Toolbar::draw() {
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoScrollbar;
    window_flags |= ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoNav;

    glm::ivec2 size(mApplication.getWindowSize().x + 2.0f - mApplication.getSidePane().getWidth(), mHeight);

    ImGui::SetNextWindowPos(glm::ivec2(-1, 0));
    ImGui::SetNextWindowSize(size);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ci::ColorA::hex(0xFFFFFF));
    ImGui::PushStyleColor(ImGuiCol_Border, ci::ColorA::hex(0xEDEDED));
    ImGui::PushStyleColor(ImGuiCol_Text, ci::ColorA::hex(0x1C2A35));
    ImGui::PushStyleColor(ImGuiCol_Button, ci::ColorA::zero());
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ci::ColorA::hex(0xCFD5DA));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ci::ColorA::hex(0x017BDA));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, glm::vec2(0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    ImGui::Begin("##toolbar", nullptr, window_flags);

    drawButton(0, ICON_MD_FOLDER_OPEN);
    drawSeparator();
    drawButton(1, ICON_MD_UNDO);
    drawButton(2, ICON_MD_REDO);
    drawSeparator();
    drawButton(3, ICON_MD_NETWORK_CELL);
    drawButton(4, ICON_MD_BRUSH);
    drawButton(5, ICON_MD_FORMAT_COLOR_FILL);
    drawButton(6, ICON_MD_TEXT_FIELDS);
    drawButton(7, ICON_MD_TEXTURE);
    drawSeparator();
    drawButton(8, ICON_MD_VISIBILITY);
    drawButton(9, ICON_MD_SETTINGS);
    drawButton(10, ICON_MD_INFO_OUTLINE);
    drawSeparator();
    drawDemoWindowButton();

    ImGui::End();

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(6);
}

void Toolbar::drawSeparator() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    glm::ivec2 cursorPos = ImGui::GetCursorScreenPos();
    drawList->AddLine(cursorPos, cursorPos + glm::ivec2(0, mHeight), ImColor(ci::ColorA::hex(0xEDEDED)));
    ImGui::SetCursorScreenPos(cursorPos + glm::ivec2(1, 0));
}

void Toolbar::drawButton(std::size_t index, const char* text) {
    std::size_t active = mSelectedButtonIndex;
    if(index == active) {
        ImGui::PushStyleColor(ImGuiCol_Text, ci::ColorA::hex(0xFFFFFF));
        ImGui::PushStyleColor(ImGuiCol_Button, ci::ColorA::hex(0x017BDA));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ci::ColorA::hex(0x017BDA));
    }
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, glm::vec2(0.5f, 0.76f));
    ImGui::Button(text, glm::ivec2(static_cast<int>(mHeight)));
    if(ImGui::IsItemActive()) {
        mSelectedButtonIndex = index;
    }
    ImGui::PopStyleVar();
    ImGui::SameLine(0.f, 0.f);
    if(index == active) {
        ImGui::PopStyleColor(3);
    }
}

void Toolbar::drawDemoWindowButton() {
    bool active = mApplication.isDemoWindowShown();
    if(active) {
        ImGui::PushStyleColor(ImGuiCol_Text, ci::ColorA::hex(0xFFFFFF));
        ImGui::PushStyleColor(ImGuiCol_Button, ci::ColorA::hex(0x017BDA));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ci::ColorA::hex(0x017BDA));
    }
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, glm::vec2(0.5f, 0.76f));
    if(ImGui::Button(ICON_MD_CHILD_FRIENDLY, glm::ivec2(static_cast<int>(mHeight)))) {
        mApplication.showDemoWindow(!active);
    }
    ImGui::PopStyleVar();
    ImGui::SameLine(0.f, 0.f);
    if(active) {
        ImGui::PopStyleColor(3);
    }
}
}