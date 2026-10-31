// ui.h | C++17 | lavender.gg | app state + ImGui UI entry point
#pragma once
#include <imgui.h>
#include <windows.h>
#include <string>
#include "bridge.h"
#include "capture.h"

// screen-space rect of the custom title bar, updated every frame, read by WM_NCHITTEST
// in main.cpp so the OS lets you drag this borderless window by its drawn title bar.
struct TitleBarRegion { ImVec2 min{0,0}, max{0,0}; };
extern TitleBarRegion g_titlebar_region;

struct AppState {
    bool visible = true;
    bool request_close = false;
    int toggle_key = VK_INSERT;
    int close_key = VK_END;

    // style
    ImVec4 accent = ImVec4(0.63f, 0.48f, 1.0f, 1.0f);
    float shimmer_intensity = 0.35f;
    float shimmer_speed = 0.6f;
    bool animations_enabled = true;
    float ui_transparency = 0.93f;

    // visuals -> pushed to RobloxBridge::set_visuals, applied in-engine by the
    // Roblox LocalScript via Highlight / BillboardGui / Beam instances
    bool v_player_labels = true;
    bool v_object_highlight = true;
    bool v_tracers = false;
    bool v_show_distance = true;
    ImVec4 tracer_color = ImVec4(1.0f, 0.65f, 1.0f, 1.0f);
    ImVec4 highlight_color = ImVec4(0.74f, 0.55f, 1.0f, 1.0f);
    float visuals_opacity = 0.85f;

    // camera
    float fov = 70.0f;
    bool camera_preview = false;

    // movement testing -> routed as server-validated commands
    float walkspeed = 16.0f;
    float jumppower = 50.0f;

    // admin -> routed as server-validated commands
    char teleport_waypoint[64] = "Spawn";

    int active_tab = 0;
    std::string status_line = "Disconnected";
};

void ui_style_init();
void ui_apply_transparency(AppState& state);
void ui_render(AppState& state, RobloxBridge& bridge, WindowCapture& capture, HWND roblox_window, float dt);

// looks for the Roblox client window. class name has changed across client versions —
// this checks both the current and legacy class name, then falls back to title match.
HWND find_roblox_window();
