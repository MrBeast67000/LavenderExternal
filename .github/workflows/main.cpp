// main.cpp | C++17 | Windows 11, MSVC | lavender.gg
// borderless always-on-top ImGui/DX11 overlay. drag/resize via WM_NCHITTEST against the
// title bar rect ui.cpp records each frame — standard custom-chrome technique, not a hack.
#include <d3d11.h>
#include <imgui.h>
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "ui.h"
#include "bridge.h"
#include "capture.h"
#include <chrono>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static ID3D11Device* g_device = nullptr;
static ID3D11DeviceContext* g_ctx = nullptr;
static IDXGISwapChain* g_swapchain = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;

static bool create_device(HWND hwnd);
static void cleanup_device();
static void create_rtv();
static void cleanup_rtv();

static const int kResizeBorder = 6;

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp)) return true;

    switch (msg) {
    case WM_SIZE:
        if (g_device && wp != SIZE_MINIMIZED) {
            cleanup_rtv();
            g_swapchain->ResizeBuffers(0, (UINT)LOWORD(lp), (UINT)HIWORD(lp), DXGI_FORMAT_UNKNOWN, 0);
            create_rtv();
        }
        return 0;
    case WM_NCHITTEST: {
        POINT pt{ LOWORD(lp), HIWORD(lp) };
        ScreenToClient(hwnd, &pt);
        RECT rc; GetClientRect(hwnd, &rc);

        bool left = pt.x < kResizeBorder;
        bool right = pt.x > rc.right - kResizeBorder;
        bool top = pt.y < kResizeBorder;
        bool bottom = pt.y > rc.bottom - kResizeBorder;

        if (top && left) return HTTOPLEFT;
        if (top && right) return HTTOPRIGHT;
        if (bottom && left) return HTBOTTOMLEFT;
        if (bottom && right) return HTBOTTOMRIGHT;
        if (left) return HTLEFT;
        if (right) return HTRIGHT;
        if (top) return HTTOP;
        if (bottom) return HTBOTTOM;

        ImVec2 p((float)pt.x, (float)pt.y);
        if (p.x >= g_titlebar_region.min.x && p.x <= g_titlebar_region.max.x &&
            p.y >= g_titlebar_region.min.y && p.y <= g_titlebar_region.max.y) {
            return HTCAPTION;
        }
        return HTCLIENT;
    }
    case WM_SYSCOMMAND:
        if ((wp & 0xFFF0) == SC_KEYMENU) return 0; // no alt-menu on a chromeless window
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int) {
    WNDCLASSEXW wc{ sizeof(WNDCLASSEXW), CS_CLASSDC, WndProc, 0, 0, hInst,
        nullptr, nullptr, nullptr, nullptr, L"lavender_gg_class", nullptr };
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        wc.lpszClassName, L"lavender.gg",
        WS_POPUP | WS_THICKFRAME,
        100, 100, 940, 1180,
        nullptr, nullptr, wc.hInstance, nullptr);
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

    if (!create_device(hwnd)) { cleanup_device(); UnregisterClassW(wc.lpszClassName, hwnd ? wc.hInstance : nullptr); return 1; }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ui_style_init();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_device, g_ctx);

    RobloxBridge bridge;
    bridge.start();

    WindowCapture capture;
    capture.init(g_device, g_ctx);

    AppState state;
    ui_apply_transparency(state);

    auto last_time = std::chrono::steady_clock::now();
    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - last_time).count();
        last_time = now;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        HWND roblox_hwnd = find_roblox_window();
        ui_render(state, bridge, capture, roblox_hwnd, dt);
        if (state.request_close) done = true;

        ImGui::Render();
        const float clear[4] = { 0, 0, 0, 0 };
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_swapchain->Present(1, 0);
    }

    bridge.stop();
    capture.shutdown();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    cleanup_device();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

static bool create_device(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL level;
    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        levels, 1, D3D11_SDK_VERSION, &sd,
        &g_swapchain, &g_device, &level, &g_ctx);
    if (FAILED(hr)) return false;
    create_rtv();
    return true;
}

static void cleanup_device() {
    cleanup_rtv();
    if (g_swapchain) { g_swapchain->Release(); g_swapchain = nullptr; }
    if (g_ctx) { g_ctx->Release(); g_ctx = nullptr; }
    if (g_device) { g_device->Release(); g_device = nullptr; }
}

static void create_rtv() {
    ID3D11Texture2D* back_buffer = nullptr;
    g_swapchain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
    if (back_buffer) {
        g_device->CreateRenderTargetView(back_buffer, nullptr, &g_rtv);
        back_buffer->Release();
    }
}

static void cleanup_rtv() {
    if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
}
