#pragma once

#include "webgpu-utils.h"

class Application final
{
public:
    Application() = default;

    bool Initialize();

    void Run();

    bool IsRunning() const;

    void Cleanup();

private:
    GLFWwindow* window_ = nullptr;
    WGPUDevice device_ = nullptr;
    WGPUQueue queue_ = nullptr;
    WGPUSurface surface_ = nullptr;

    WGPUTextureView GetNextSurfaceTextureView();
};