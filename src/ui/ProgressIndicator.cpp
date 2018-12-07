#include "ProgressIndicator.h"

namespace pepr3d {

void ProgressIndicator::draw() {
    if(mGeometry == nullptr) {
        return;
    }
    if(!ImGui::IsPopupOpen("##progressindicator")) {
        ImGui::OpenPopup("##progressindicator");
    }
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoScrollbar;
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
    ImGui::PushStyleColor(ImGuiCol_Border, ci::ColorA::hex(0xEDEDED));
    ImGui::PushStyleColor(ImGuiCol_Text, ci::ColorA::hex(0x1C2A35));
    ImGui::PushStyleColor(ImGuiCol_Separator, ci::ColorA::hex(0xEDEDED));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, glm::vec2(12.0f));
    if(ImGui::BeginPopupModal("##progressindicator", nullptr, window_flags)) {
        drawSpinner("##progressindicator#spinner");
        ImGui::SameLine();
        ImGui::Text("Please wait, Pepr3D is processing the geometry...");

        ImGui::Separator();

        const auto& progress = mGeometry->getProgress();

        drawStatus("Importing render geometry...", progress.importRenderPercentage, false);
        drawStatus("Importing compute geometry...", progress.importComputePercentage, false);
        drawStatus("Generating buffers...", progress.buffersPercentage, false);
        drawStatus("Building AABB tree...", progress.aabbTreePercentage, true);
        drawStatus("Building polyhedron...", progress.polyhedronPercentage, true);

        drawStatus("Creating scene...", progress.createScenePercentage, true);
        drawStatus("Exporting geometry...", progress.exportFilePercentage, false);

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);
}

void ProgressIndicator::drawSpinner(const char* label) {
    // https://github.com/ocornut/imgui/issues/1901 (modified)

    const ImU32 color = ImGui::ColorConvertFloat4ToU32(ci::ColorA::hex(0x017BDA));
    float radius = 10.0f;
    float thickness = 4.0f;

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if(window->SkipItems) {
        return;
    }

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size((radius)*2, (radius + style.FramePadding.y) * 2);

    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ImGui::ItemSize(bb, style.FramePadding.y);
    if(!ImGui::ItemAdd(bb, id)) {
        return;
    }

    // Render
    window->DrawList->PathClear();

    int num_segments = 30;
    int start = 0;

    const float a_min = IM_PI * 2.0f * ((float)start) / (float)num_segments;
    const float a_max = IM_PI * 2.0f * ((float)num_segments - 3) / (float)num_segments;

    const ImVec2 centre = ImVec2(pos.x + radius, pos.y + radius + style.FramePadding.y);

    for(int i = 0; i < num_segments; i++) {
        const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        window->DrawList->PathLineTo(ImVec2(centre.x + static_cast<float>(std::cos(a + g.Time * 8.0)) * radius,
                                            centre.y + static_cast<float>(std::sin(a + g.Time * 8.0)) * radius));
    }

    window->DrawList->PathStroke(color, false, thickness);
}

void ProgressIndicator::drawStatus(const std::string& label, float progress, bool isIndeterminate) {
    if(progress < 0.0f || progress >= 1.0f) {
        return;
    }

    ImGui::Text(label.c_str());

    const ImU32 bg_col = ImGui::ColorConvertFloat4ToU32(ci::ColorA::hex(0xD9D9D9));
    const ImU32 fg_col = ImGui::ColorConvertFloat4ToU32(ci::ColorA::hex(0x017BDA));

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if(window->SkipItems) {
        return;
    }

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.c_str());

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size(ImGui::GetContentRegionAvailWidth(), 6);
    size.x -= style.FramePadding.x * 2;

    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ImGui::ItemSize(bb, style.FramePadding.y);
    if(!ImGui::ItemAdd(bb, id)) {
        return;
    }

    // Render

    float progressStart, progressEnd;

    if(isIndeterminate) {
        progressStart = static_cast<float>(std::fmod(g.Time * 500.0f, size.x + 50.0f)) - 50.0f;
        progressEnd = progressStart + 50.0f;
    } else {
        progressStart = 0.0f;
        progressEnd = size.x * progress;
    }

    window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + size.x, bb.Max.y), bg_col);
    window->DrawList->AddRectFilled(ImVec2(bb.Min.x + std::max(0.0f, progressStart), bb.Min.y),
                                    ImVec2(pos.x + std::min(size.x, progressEnd), bb.Max.y), fg_col);
}

}  // namespace pepr3d