// ui.cpp | C++17 | lavender.gg
#include "ui.h"
#include <imgui_internal.h>
#include <chrono>
#include <cmath>
#include <fstream>
#include <vector>

TitleBarRegion g_titlebar_region;

static float g_time = 0.0f;

HWND find_roblox_window() {
    HWND h = FindWindowA("WINDOWSCLIENT", nullptr);     // current Roblox client class
    if (!h) h = FindWindowA("ROBLOXWINDOWCLASS", nullptr); // legacy class name
    if (!h) h = FindWindowA(nullptr, "Roblox");            // title fallback
    return h; // may legitimately be null if Roblox isn't running — callers check this
}

void ui_style_init() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 16.0f);
    if (io.Fonts->Fonts.empty()) io.Fonts->AddFontDefault();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.TabRounding = 5.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.ItemSpacing = ImVec2(10, 8);
    style.FramePadding = ImVec2(10, 6);
    style.WindowPadding = ImVec2(16, 14);

    ImVec4* c = style.Colors;
    c[ImGuiCol_WindowBg]          = ImVec4(0.04f, 0.035f, 0.05f, 0.96f);
    c[ImGuiCol_ChildBg]           = ImVec4(0.06f, 0.055f, 0.075f, 1.0f);
    c[ImGuiCol_Border]            = ImVec4(0.75f, 0.73f, 0.80f, 0.35f);
    c[ImGuiCol_FrameBg]           = ImVec4(0.09f, 0.08f, 0.11f, 1.0f);
    c[ImGuiCol_FrameBgHovered]    = ImVec4(0.14f, 0.11f, 0.19f, 1.0f);
    c[ImGuiCol_FrameBgActive]     = ImVec4(0.18f, 0.14f, 0.26f, 1.0f);
    c[ImGuiCol_Header]            = ImVec4(0.30f, 0.22f, 0.50f, 0.55f);
    c[ImGuiCol_HeaderHovered]     = ImVec4(0.40f, 0.30f, 0.65f, 0.70f);
    c[ImGuiCol_HeaderActive]      = ImVec4(0.45f, 0.34f, 0.72f, 0.85f);
    c[ImGuiCol_Button]            = ImVec4(0.14f, 0.11f, 0.19f, 1.0f);
    c[ImGuiCol_ButtonHovered]     = ImVec4(0.40f, 0.30f, 0.65f, 0.65f);
    c[ImGuiCol_ButtonActive]      = ImVec4(0.50f, 0.38f, 0.80f, 0.85f);
    c[ImGuiCol_SliderGrab]        = ImVec4(0.63f, 0.48f, 1.0f, 1.0f);
    c[ImGuiCol_SliderGrabActive]  = ImVec4(0.75f, 0.60f, 1.0f, 1.0f);
    c[ImGuiCol_CheckMark]         = ImVec4(0.72f, 0.58f, 1.0f, 1.0f);
    c[ImGuiCol_Tab]               = ImVec4(0.10f, 0.08f, 0.14f, 1.0f);
    c[ImGuiCol_TabHovered]        = ImVec4(0.40f, 0.30f, 0.65f, 0.75f);
    c[ImGuiCol_TabActive]         = ImVec4(0.30f, 0.22f, 0.50f, 1.0f);
    c[ImGuiCol_Text]              = ImVec4(0.92f, 0.91f, 0.95f, 1.0f);
    c[ImGuiCol_TextDisabled]      = ImVec4(0.55f, 0.53f, 0.60f, 1.0f);
    c[ImGuiCol_Separator]         = ImVec4(0.75f, 0.73f, 0.80f, 0.25f);
}

void ui_apply_transparency(AppState& state) {
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = state.ui_transparency;
}

// animated silver shimmer across the top of the window — purely decorative,
// cost is one diagonal gradient quad per frame, cheap.
static void draw_shimmer(ImDrawList* dl, ImVec2 win_pos, ImVec2 win_size, const AppState& s) {
    if (!s.animations_enabled || s.shimmer_intensity <= 0.001f) return;
    float band_w = win_size.x * 0.5f;
    float t = fmodf(g_time * s.shimmer_speed, 2.0f);
    float x = win_pos.x - band_w + t * (win_size.x + band_w);
    ImU32 col_a = IM_COL32(255, 255, 255, 0);
    ImU32 col_b = IM_COL32(255, 255, 255, (int)(s.shimmer_intensity * 60));
    dl->AddRectFilledMultiColor(
        ImVec2(x, win_pos.y), ImVec2(x + band_w, win_pos.y + 36),
        col_a, col_b, col_b, col_a);
}

static void section_header(const char* label, const AppState& s) {
    ImGui::TextColored(s.accent, "%s", label);
    ImGui::Separator();
    ImGui::Spacing();
}

static bool key_pressed_edge(int vk) {
    static bool held[256] = {};
    bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool edge = down && !held[vk & 0xFF];
    held[vk & 0xFF] = down;
    return edge;
}

static void push_visuals(AppState& s, RobloxBridge& bridge) {
    nlohmann::json v = {
        {"playerLabels", s.v_player_labels},
        {"objectHighlight", s.v_object_highlight},
        {"tracers", s.v_tracers},
        {"tracerColor", {(int)(s.tracer_color.x*255), (int)(s.tracer_color.y*255), (int)(s.tracer_color.z*255)}},
        {"highlightColor", {(int)(s.highlight_color.x*255), (int)(s.highlight_color.y*255), (int)(s.highlight_color.z*255)}},
        {"opacity", s.visuals_opacity},
        {"showDistance", s.v_show_distance},
        {"fov", s.fov}
    };
    bridge.set_visuals(v);
}

static void draw_titlebar(AppState& state) {
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 full_w = ImVec2(ImGui::GetWindowSize().x - ImGui::GetStyle().WindowPadding.x * 2, 30);
    ImGui::InvisibleButton("##titlebar_drag", full_w);
    g_titlebar_region.min = p0;
    g_titlebar_region.max = ImVec2(p0.x + full_w.x, p0.y + full_w.y);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddText(ImVec2(p0.x, p0.y + 4), ImGui::GetColorU32(state.accent), "lavender.gg");

    ImGui::SetCursorScreenPos(ImVec2(p0.x + full_w.x - 56, p0.y));
    if (ImGui::Button("_", ImVec2(24, 24))) state.visible = false;
    ImGui::SameLine();
    if (ImGui::Button("x", ImVec2(24, 24))) state.request_close = true;

    ImGui::SetCursorScreenPos(ImVec2(p0.x, p0.y + full_w.y + 6));
    ImGui::Separator();
}

static void tab_home(AppState& s, RobloxBridge& bridge) {
    section_header("Welcome to lavender.gg", s);
    ImGui::TextDisabled("Your ultimate Roblox testing companion.");
    ImGui::Spacing();

    bool connected = bridge.has_fresh_state();
    nlohmann::json state = bridge.get_latest_state();

    ImGui::BeginChild("status_card", ImVec2(0, 90), true);
    ImGui::TextColored(connected ? ImVec4(0.4f,0.9f,0.5f,1) : ImVec4(0.9f,0.4f,0.4f,1),
        connected ? "Connected" : "Waiting for Roblox...");
    ImGui::TextDisabled("Bridge: 127.0.0.1:7777 — requires HttpService enabled in Studio");
    if (connected && state.contains("session")) {
        auto& sess = state["session"];
        ImGui::Text("Place: %s   JobId: %s   FPS: %s",
            sess.value("placeId", "?").c_str(),
            sess.value("jobId", "?").c_str(),
            std::to_string(sess.value("fps", 0.0)).c_str());
    }
    ImGui::EndChild();

    ImGui::Spacing();
    section_header("Player", s);
    if (connected && state.contains("player")) {
        auto& p = state["player"];
        ImGui::Text("Name: %s", p.value("name", "?").c_str());
        double hp = p.value("health", 0.0), maxhp = p.value("maxHealth", 100.0);
        ImGui::Text("Health: %.0f / %.0f", hp, maxhp);
        ImGui::ProgressBar((float)(maxhp > 0 ? hp/maxhp : 0), ImVec2(-1, 14));
    } else {
        ImGui::TextDisabled("No player data yet.");
    }

    ImGui::Spacing();
    section_header("Notifications", s);
    if (connected && state.contains("notifications")) {
        for (auto& n : state["notifications"]) {
            ImGui::BulletText("%s", n.get<std::string>().c_str());
        }
    } else {
        ImGui::TextDisabled("None.");
    }
}

static void tab_visuals(AppState& s, RobloxBridge& bridge) {
    section_header("Visuals (applied in-engine via Highlight / BillboardGui)", s);
    bool changed = false;
    changed |= ImGui::Checkbox("Player labels", &s.v_player_labels);
    changed |= ImGui::Checkbox("Object highlighting", &s.v_object_highlight);
    changed |= ImGui::Checkbox("Tracers", &s.v_tracers);
    changed |= ImGui::Checkbox("Show distance", &s.v_show_distance);
    ImGui::Spacing();
    changed |= ImGui::ColorEdit3("Tracer color", &s.tracer_color.x);
    changed |= ImGui::ColorEdit3("Highlight color", &s.highlight_color.x);
    changed |= ImGui::SliderFloat("Opacity", &s.visuals_opacity, 0.0f, 1.0f);
    if (changed) push_visuals(s, bridge);
}

static void tab_camera(AppState& s, RobloxBridge& bridge, WindowCapture& capture, HWND roblox_window) {
    section_header("Camera", s);
    if (ImGui::SliderFloat("Field of view", &s.fov, 30.0f, 120.0f, "%.0f")) push_visuals(s, bridge);
    ImGui::TextDisabled("FOV is applied client-side in Roblox (Camera.FieldOfView).");
    ImGui::Spacing();
    ImGui::Checkbox("Camera preview", &s.camera_preview);
    ImGui::SameLine();
    ImGui::TextDisabled("(screen capture of the Roblox window, not a Roblox API)");

    if (s.camera_preview) {
        if (!roblox_window) {
            ImGui::TextColored(ImVec4(0.9f,0.5f,0.5f,1), "Roblox window not found.");
        } else {
            ID3D11ShaderResourceView* srv = capture.capture_window(roblox_window);
            if (srv) {
                ImGui::Image((ImTextureID)srv, ImVec2(360, 202));
            } else {
                ImGui::TextDisabled("No frame yet.");
            }
        }
    }
}

static void tab_movement(AppState& s, RobloxBridge& bridge) {
    section_header("Movement testing", s);
    ImGui::SliderFloat("WalkSpeed", &s.walkspeed, 0.0f, 200.0f, "%.0f");
    ImGui::SliderFloat("JumpPower", &s.jumppower, 0.0f, 300.0f, "%.0f");
    if (ImGui::Button("Apply", ImVec2(120, 0))) {
        bridge.push_command({{"type", "setmovement"}, {"walkspeed", s.walkspeed}, {"jumppower", s.jumppower}});
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset to default", ImVec2(160, 0))) {
        s.walkspeed = 16.0f; s.jumppower = 50.0f;
        bridge.push_command({{"type", "setmovement"}, {"walkspeed", 16.0}, {"jumppower", 50.0}});
    }
    ImGui::TextDisabled("Routed through a server-validated RemoteEvent — not set directly.");
}

static void tab_admin(AppState& s, RobloxBridge& bridge) {
    section_header("Admin (server-validated, your UserId only)", s);
    if (ImGui::Button("Heal", ImVec2(120, 0))) bridge.push_command({{"type", "heal"}});
    ImGui::SameLine();
    if (ImGui::Button("Respawn", ImVec2(120, 0))) bridge.push_command({{"type", "respawn"}});

    ImGui::Spacing();
    ImGui::InputText("Waypoint name", s.teleport_waypoint, sizeof(s.teleport_waypoint));
    ImGui::TextDisabled("Matches a Part under workspace.LavenderWaypoints in your test place.");
    if (ImGui::Button("Teleport", ImVec2(120, 0))) {
        bridge.push_command({{"type", "teleport"}, {"waypoint", std::string(s.teleport_waypoint)}});
    }
    ImGui::TextDisabled("Every command is checked server-side against your UserId before it runs.");
}

static void tab_settings(AppState& s) {
    section_header("Settings", s);
    ImGui::ColorEdit3("Accent color", &s.accent.x);
    ImGui::SliderFloat("Shimmer intensity", &s.shimmer_intensity, 0.0f, 1.0f);
    ImGui::SliderFloat("Shimmer speed", &s.shimmer_speed, 0.0f, 2.0f);
    ImGui::Checkbox("UI animations", &s.animations_enabled);
    if (ImGui::SliderFloat("UI transparency", &s.ui_transparency, 0.4f, 1.0f)) ui_apply_transparency(s);

    ImGui::Spacing();
    ImGui::Text("Toggle key: INSERT    Close key: END");

    ImGui::Spacing();
    if (ImGui::Button("Save config", ImVec2(130, 0))) {
        nlohmann::json j = {
            {"accent", {s.accent.x, s.accent.y, s.accent.z}},
            {"shimmer_intensity", s.shimmer_intensity},
            {"shimmer_speed", s.shimmer_speed},
            {"animations_enabled", s.animations_enabled},
            {"ui_transparency", s.ui_transparency}
        };
        std::ofstream f("lavender_config.json");
        f << j.dump(2);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load config", ImVec2(130, 0))) {
        std::ifstream f("lavender_config.json");
        if (f) {
            nlohmann::json j; f >> j;
            auto a = j.value("accent", std::vector<float>{s.accent.x, s.accent.y, s.accent.z});
            s.accent = ImVec4(a[0], a[1], a[2], 1.0f);
            s.shimmer_intensity = j.value("shimmer_intensity", s.shimmer_intensity);
            s.shimmer_speed = j.value("shimmer_speed", s.shimmer_speed);
            s.animations_enabled = j.value("animations_enabled", s.animations_enabled);
            s.ui_transparency = j.value("ui_transparency", s.ui_transparency);
            ui_apply_transparency(s);
        }
    }
}

void ui_render(AppState& s, RobloxBridge& bridge, WindowCapture& capture, HWND roblox_window, float dt) {
    g_time += dt;

    if (key_pressed_edge(s.toggle_key)) s.visible = !s.visible;
    if (key_pressed_edge(s.close_key)) s.request_close = true;
    if (!s.visible) return;

    ImGui::SetNextWindowSize(ImVec2(940, 1180), ImGuiCond_FirstUseEver);
    ImGui::Begin("##lavender_root", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);

    ImVec2 win_pos = ImGui::GetWindowPos();
    ImVec2 win_size = ImGui::GetWindowSize();
    draw_shimmer(ImGui::GetWindowDrawList(), win_pos, win_size, s);

    draw_titlebar(s);
    ImGui::Spacing();

    static const char* tabs[] = {"Home", "Visuals", "Camera", "Movement", "Admin", "Settings"};
    ImGui::BeginChild("##sidebar", ImVec2(160, -1), true);
    for (int i = 0; i < 6; ++i) {
        bool sel = (s.active_tab == i);
        if (sel) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
        if (ImGui::Button(tabs[i], ImVec2(-1, 34))) s.active_tab = i;
        if (sel) ImGui::PopStyleColor();
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("##content", ImVec2(0, -1), true);
    switch (s.active_tab) {
        case 0: tab_home(s, bridge); break;
        case 1: tab_visuals(s, bridge); break;
        case 2: tab_camera(s, bridge, capture, roblox_window); break;
        case 3: tab_movement(s, bridge); break;
        case 4: tab_admin(s, bridge); break;
        case 5: tab_settings(s); break;
    }
    ImGui::EndChild();

    ImGui::End();
}
