// capture.h | C++17 | lavender.gg | Windows DXGI Output Duplication, cropped to the Roblox
// window's rect via GetWindowRect. Same documented capture API OBS/Discord use for screen
// share — monitor-level framebuffer grab, no process memory access, no injection.
#pragma once
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <windows.h>

using Microsoft::WRL::ComPtr;

class WindowCapture {
public:
    bool init(ID3D11Device* device, ID3D11DeviceContext* ctx);
    void shutdown();

    // grabs the current frame, crops to target's window rect, returns an SRV ready
    // for ImGui::Image(). returns the previous frame's SRV on a duplication timeout
    // (normal — the desktop hasn't changed), nullptr if the window/device isn't valid.
    ID3D11ShaderResourceView* capture_window(HWND target);

private:
    bool ensure_duplication();

    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* ctx_ = nullptr;
    ComPtr<IDXGIOutputDuplication> duplication_;
    ComPtr<ID3D11Texture2D> cropped_tex_;
    ComPtr<ID3D11ShaderResourceView> cropped_srv_;
    int out_w_ = 0, out_h_ = 0;
};
