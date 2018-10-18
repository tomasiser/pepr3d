#pragma once

#include "CinderImGui.h"
#include "IconsMaterialDesign.h"
#include "UiStateStore.h"
// #include "MainApplication.h"

namespace pepr3d {

class MainApplication;

class Toolbar {
   public:
    MainApplication& app;
    Toolbar(MainApplication& app) : app(app) {}

    float foo() {
        return app.getWindow()->getAspectRatio();
    }

   private:
    int number;
};

namespace {
void drawToolbarSeparator(float height) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    glm::ivec2 cursorPos = ImGui::GetCursorScreenPos();
    drawList->AddLine(cursorPos, cursorPos + glm::ivec2(0, height), ImColor(ci::ColorA::hex(0xEDEDED)));
    ImGui::SetCursorScreenPos(cursorPos + glm::ivec2(1, 0));
}
}  // namespace

inline void drawToolbarButton(std::size_t index, const char* text, ToolbarState& state) {
    std::size_t active = state.selectedButtonIndex;
    if(index == active) {
        ImGui::PushStyleColor(ImGuiCol_Text, ci::ColorA::hex(0xFFFFFF));
        ImGui::PushStyleColor(ImGuiCol_Button, ci::ColorA::hex(0x017BDA));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ci::ColorA::hex(0x017BDA));
    }
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, glm::vec2(0.5f, 0.76f));
    ImGui::Button(text, glm::ivec2(static_cast<int>(state.height)));
    if(ImGui::IsItemActive()) {
        state.selectedButtonIndex = index;
    }
    ImGui::PopStyleVar();
    ImGui::SameLine(0.f, 0.f);
    if(index == active) {
        ImGui::PopStyleColor(3);
    }
}

inline void drawDemoWindowButton(UiStateStore& state) {
    bool active = state.mainWindow.showDemoWindow;
    if(active) {
        ImGui::PushStyleColor(ImGuiCol_Text, ci::ColorA::hex(0xFFFFFF));
        ImGui::PushStyleColor(ImGuiCol_Button, ci::ColorA::hex(0x017BDA));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ci::ColorA::hex(0x017BDA));
    }
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, glm::vec2(0.5f, 0.76f));
    if(ImGui::Button(ICON_MD_CHILD_FRIENDLY, glm::ivec2(static_cast<int>(state.toolbar.height)))) {
        state.mainWindow.showDemoWindow = !state.mainWindow.showDemoWindow;
    }
    ImGui::PopStyleVar();
    ImGui::SameLine(0.f, 0.f);
    if(active) {
        ImGui::PopStyleColor(3);
    }
}

inline void drawToolbar(UiStateStore& state) {
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoScrollbar;
    window_flags |= ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoNav;

    glm::ivec2 size(state.mainWindow.size.x + 2.0f - state.sidePane.width, state.toolbar.height);

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

    drawToolbarButton(0, ICON_MD_FOLDER_OPEN, state.toolbar);
    drawToolbarSeparator(static_cast<float>(size.y));
    drawToolbarButton(1, ICON_MD_UNDO, state.toolbar);
    drawToolbarButton(2, ICON_MD_REDO, state.toolbar);
    drawToolbarSeparator(static_cast<float>(size.y));
    drawToolbarButton(3, ICON_MD_NETWORK_CELL, state.toolbar);
    drawToolbarButton(4, ICON_MD_BRUSH, state.toolbar);
    drawToolbarButton(5, ICON_MD_FORMAT_COLOR_FILL, state.toolbar);
    drawToolbarButton(6, ICON_MD_TEXT_FIELDS, state.toolbar);
    drawToolbarButton(7, ICON_MD_TEXTURE, state.toolbar);
    drawToolbarSeparator(static_cast<float>(size.y));
    drawToolbarButton(8, ICON_MD_VISIBILITY, state.toolbar);
    drawToolbarButton(9, ICON_MD_SETTINGS, state.toolbar);
    drawToolbarButton(10, ICON_MD_INFO_OUTLINE, state.toolbar);
    drawToolbarSeparator(static_cast<float>(size.y));
    drawDemoWindowButton(state);

    ImGui::End();

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(6);
}

}  // namespace pepr3d