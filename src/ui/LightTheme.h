#pragma once

#include "peprimgui.h"

namespace {

/// Applies our light ImGui colors.
inline void applyColors(ImVec4* colors) {
    colors[ImGuiCol_Text] = ci::ColorA::hex(0x1C2A35);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.988f, 0.988f, 0.988f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.92f);
    colors[ImGuiCol_Border] = ci::ColorA::hex(0xE5E5E5);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.98f, 0.98f, 0.98f, 0.88f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.94f, 0.94f, 0.94f, 0.20f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.95f, 0.95f, 0.95f, 0.92f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.67f, 0.67f, 0.67f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.72f, 0.72f, 0.72f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.57f, 0.57f, 0.57f, 0.34f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.24f, 0.24f, 0.24f, 0.34f);
    colors[ImGuiCol_Button] = ImVec4(0.97f, 0.97f, 0.97f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.93f, 0.93f, 0.93f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.84f, 0.84f, 0.84f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.84f, 0.84f, 0.84f, 1.00f);
    colors[ImGuiCol_Separator] = ci::ColorA::hex(0xE5E5E5);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.70f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.90f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.37f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.00f, 0.00f, 0.00f, 0.47f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.69f, 0.56f, 0.12f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.64f, 0.50f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.37f, 0.22f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.46f, 0.61f, 1.00f, 0.61f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
}

}  // namespace

namespace pepr3d {

/// Applies our light ImGui theme.
inline void applyLightTheme(ImGuiStyle& style) {
    applyColors(style.Colors);
}

}  // namespace pepr3d