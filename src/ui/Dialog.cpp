#include "Dialog.h"

namespace pepr3d {

bool Dialog::draw() const {
    bool isAccepted = false;
    if(!ImGui::IsPopupOpen("##dialog")) {
        ImGui::OpenPopup("##dialog");
    }
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoNav;
    const ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2.0f, io.DisplaySize.y / 2.0f), ImGuiCond_Always,
                            ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, -1.0f));
    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ci::ColorA::hex(0xFFFFFF));
    ImGui::PushStyleColor(ImGuiCol_Border, ci::ColorA::hex(0xE5E5E5));
    ImGui::PushStyleColor(ImGuiCol_Text, ci::ColorA::hex(0x1C2A35));
    ImGui::PushStyleColor(ImGuiCol_Separator, ci::ColorA::hex(0xE5E5E5));
    ImGui::PushStyleColor(ImGuiCol_Button, ci::ColorA::zero());
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ci::ColorA::hex(0xCFD5DA));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ci::ColorA::hex(0xA3B2BF));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, glm::vec2(12.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, glm::vec2(8.0f, 6.0f));
    if(ImGui::BeginPopupModal("##dialog", nullptr, window_flags)) {
        auto captionColor = ci::ColorA::hex(0xEB5757);  // red
        if(mType == DialogType::Warning) {
            captionColor = ci::ColorA::hex(0xF2994A);  // orange
        } else if(mType == DialogType::Information) {
            captionColor = ci::ColorA::hex(0x017BDA);  // blue
        }
        ImGui::PushStyleColor(ImGuiCol_Text, captionColor);
        ImGui::TextWrapped(mCaption.c_str());
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextWrapped(mMessage.c_str());

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        isAccepted = ImGui::Button(mAcceptButton.c_str(), glm::ivec2(ImGui::GetContentRegionAvailWidth(), 33));
        ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                            (ImColor)ci::ColorA::hex(0xE5E5E5));

        if(isAccepted) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(7);
    return isAccepted;
}

}  // namespace pepr3d