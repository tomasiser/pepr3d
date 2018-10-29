#include "SidePane.h"
#include "MainApplication.h"
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

    ImGui::SetCursorPos(glm::ivec2(12, 22));
    ImGui::Text(currentTool.getIcon().c_str());
    ImGui::SetCursorPos(glm::ivec2(50, 15));
    ImGui::Text(currentTool.getName().c_str());

    ImGui::End();

    // ImGui::PopStyleVar(1);

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

void SidePane::drawSeparator() {
    glm::ivec2 cursorPos = ImGui::GetCursorScreenPos();
    auto* drawList = ImGui::GetWindowDrawList();
    drawList->PushClipRectFullScreen();
    drawList->AddLine(glm::ivec2(mApplication.getWindowSize().x - mWidth, cursorPos.y),
                      glm::ivec2(mApplication.getWindowSize().x, cursorPos.y), (ImColor)ci::ColorA::hex(0xEDEDED));
    drawList->PopClipRect();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);
}

}  // namespace pepr3d