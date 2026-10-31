// capture.cpp | C++17 | lavender.gg
#include "capture.h"
#include <algorithm>

bool WindowCapture::init(ID3D11Device* device, ID3D11DeviceContext* ctx) {
    device_ = device;
    ctx_ = ctx;
    return ensure_duplication();
}

bool WindowCapture::ensure_duplication() {
    if (duplication_) return true;
    if (!device_) return false;

    ComPtr<IDXGIDevice> dxgi_device;
    if (FAILED(device_->QueryInterface(IID_PPV_ARGS(&dxgi_device)))) return false;

    ComPtr<IDXGIAdapter> adapter;
    if (FAILED(dxgi_device->GetAdapter(&adapter))) return false;

    ComPtr<IDXGIOutput> output;
    // output index 0 = primary monitor. FILL: if Roblox runs on a secondary monitor,
    // enumerate outputs and pick the one returned by MonitorFromWindow(target, ...)
    // instead of hardcoding 0.
    if (FAILED(adapter->EnumOutputs(0, &output))) return false;

    ComPtr<IDXGIOutput1> output1;
    if (FAILED(output.As(&output1))) return false;

    return SUCCEEDED(output1->DuplicateOutput(device_, &duplication_));
}

void WindowCapture::shutdown() {
    duplication_.Reset();
    cropped_tex_.Reset();
    cropped_srv_.Reset();
}

ID3D11ShaderResourceView* WindowCapture::capture_window(HWND target) {
    if (!target || !IsWindow(target)) return nullptr;
    if (!ensure_duplication()) return nullptr;

    ComPtr<IDXGIResource> desktop_resource;
    DXGI_OUTDUPL_FRAME_INFO frame_info{};
    HRESULT hr = duplication_->AcquireNextFrame(0, &frame_info, &desktop_resource);

    if (hr == DXGI_ERROR_WAIT_TIMEOUT) return cropped_srv_.Get(); // desktop unchanged
    if (FAILED(hr)) { duplication_.Reset(); return nullptr; }

    ComPtr<ID3D11Texture2D> frame_tex;
    desktop_resource.As(&frame_tex);

    RECT wr{};
    GetWindowRect(target, &wr);
    int w = (std::max)(1L, wr.right - wr.left);
    int h = (std::max)(1L, wr.bottom - wr.top);

    if (!cropped_tex_ || out_w_ != w || out_h_ != h) {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = w; desc.Height = h;
        desc.MipLevels = 1; desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        device_->CreateTexture2D(&desc, nullptr, &cropped_tex_);
        device_->CreateShaderResourceView(cropped_tex_.Get(), nullptr, &cropped_srv_);
        out_w_ = w; out_h_ = h;
    }

    D3D11_BOX box{};
    box.left = (std::max)(0L, wr.left);
    box.top = (std::max)(0L, wr.top);
    box.right = box.left + w;
    box.bottom = box.top + h;
    box.front = 0; box.back = 1;
    ctx_->CopySubresourceRegion(cropped_tex_.Get(), 0, 0, 0, 0, frame_tex.Get(), 0, &box);

    duplication_->ReleaseFrame();
    return cropped_srv_.Get();
}
