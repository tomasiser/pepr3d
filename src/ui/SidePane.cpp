#include "SidePane.h"
#include "MainApplication.h"

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
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, glm::vec4(1.0f, 0.438f, 0.262f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, glm::vec2(0.0f));

    ImGui::Begin("##sidepane", nullptr, window_flags);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    glm::ivec2 cursorPos = ImGui::GetCursorScreenPos();
    drawList->AddLine(cursorPos + glm::ivec2(0, 49), cursorPos + glm::ivec2(size.x, 49),
                      ImColor(ci::ColorA::hex(0xEDEDED)));

    ImGui::SetCursorPos(glm::ivec2(12, 22));
    ImGui::Text(ICON_MD_BUILD);
    ImGui::SetCursorPos(glm::ivec2(50, 15));
    ImGui::Text("Debug");

    ImGui::End();

    // ImGui::PopStyleVar(1);

    ImGui::SetNextWindowPos(glm::ivec2(mApplication.getWindowSize().x - size.x, 49));
    ImGui::SetNextWindowSize(glm::ivec2(size.x + 1.0f, size.y - mApplication.getToolbar().getHeight() + 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, glm::vec2(12.0f));
    ImGui::Begin("##sidepane-debug", nullptr, window_flags);
    ImGui::End();

    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(6);
}

}  // namespace pepr3d