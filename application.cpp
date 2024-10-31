#include "application.h"

bool Application::Initialize()
{
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(640, 480, "Web Ocean GPU", nullptr, nullptr);

    WGPUInstance instance = wgpuCreateInstance(nullptr);

    std::cout << "Requesting adapter" << std::endl;

    surface_ = glfwGetWGPUSurface(instance, window_);
    WGPURequestAdapterOptions requestAdapterOptions = {};
    requestAdapterOptions.nextInChain = nullptr;
    requestAdapterOptions.compatibleSurface = surface_;
    WGPUAdapter adapter = requestAdapterSync(instance, &requestAdapterOptions);
    std::cout << "WGPU adapter: " << adapter << std::endl;

    wgpuInstanceRelease(instance);

    std::cout << "Requesting device" << std::endl;
    WGPUDeviceDescriptor deviceDescriptor = {};
    deviceDescriptor.nextInChain = nullptr;
    deviceDescriptor.label = "My device";
    deviceDescriptor.requiredFeatureCount = 0;
    deviceDescriptor.requiredLimits = nullptr;
    deviceDescriptor.defaultQueue.nextInChain = nullptr;
    deviceDescriptor.defaultQueue.label = "My default queue";

    deviceDescriptor.deviceLostCallback = [](WGPUDeviceLostReason reason, char const * message, void*) {
        std::cout << "Device lost: reason " << reason;
        if (message) {
            std::cout << ", message: " << message;
        }
        std::cout << std::endl;
    };
    device_ = requestDeviceSync(adapter, &deviceDescriptor);
    std::cout << "WGPU device: " << device_ << std::endl;

    auto onDeviceError = [](WGPUErrorType type, char const * message, void*) {
        std::cout << "Device error: type " << type;
        if (message) {
            std::cout << ", message: " << message;
        }
        std::cout << std::endl;
    };
    wgpuDeviceSetUncapturedErrorCallback(device_, onDeviceError, nullptr);

    queue_ = wgpuDeviceGetQueue(device_);

    WGPUSurfaceConfiguration surfaceConfiguration = {};
    surfaceConfiguration.width = 640;
    surfaceConfiguration.height = 480;
    WGPUTextureFormat surfaceFormat = wgpuSurfaceGetPreferredFormat(surface_, adapter);
    surfaceConfiguration.format = surfaceFormat;
    surfaceConfiguration.viewFormatCount = 0;
    surfaceConfiguration.viewFormats = nullptr;
    surfaceConfiguration.nextInChain = nullptr;
    surfaceConfiguration.usage = WGPUTextureUsage_RenderAttachment;
    surfaceConfiguration.device = device_;
    surfaceConfiguration.presentMode = WGPUPresentMode_Fifo;
    surfaceConfiguration.alphaMode = WGPUCompositeAlphaMode_Auto;
    wgpuSurfaceConfigure(surface_, &surfaceConfiguration);

    wgpuAdapterRelease(adapter);

    return true;
}

void Application::Run()
{
    glfwPollEvents();

    WGPUTextureView textureView = GetNextSurfaceTextureView();
    if (!textureView) return;

    WGPUCommandEncoderDescriptor commandEncoderDescriptor = {};
    commandEncoderDescriptor.nextInChain = nullptr;
    commandEncoderDescriptor.label = "My command encoder";
    WGPUCommandEncoder commandEncoder = wgpuDeviceCreateCommandEncoder(device_, &commandEncoderDescriptor);

    WGPURenderPassDescriptor renderPassDescriptor = {};
    renderPassDescriptor.nextInChain = nullptr;

    WGPURenderPassColorAttachment renderColorAttachment = {};
    renderColorAttachment.view = textureView;
    renderColorAttachment.resolveTarget = nullptr;
    renderColorAttachment.loadOp = WGPULoadOp_Clear;
    renderColorAttachment.storeOp = WGPUStoreOp_Store;
    renderColorAttachment.clearValue = { 1.0f, 0.0f, 0.0f, 1.0f };
#ifndef WEBGPU_BACKEND_WGPU
    renderColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif

    renderPassDescriptor.colorAttachmentCount = 1;
    renderPassDescriptor.colorAttachments = &renderColorAttachment;
    renderPassDescriptor.depthStencilAttachment = nullptr;
    renderPassDescriptor.timestampWrites = nullptr;

    WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);
    wgpuRenderPassEncoderEnd(renderPass);
    wgpuRenderPassEncoderRelease(renderPass);

    WGPUCommandBufferDescriptor commandBufferDescriptor = {};
    commandBufferDescriptor.nextInChain = nullptr;
    commandBufferDescriptor.label = "My command buffer";
    WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(commandEncoder, &commandBufferDescriptor);
    wgpuCommandEncoderRelease(commandEncoder);

    wgpuQueueSubmit(queue_, 1, &commandBuffer);
    wgpuCommandBufferRelease(commandBuffer);

    wgpuTextureViewRelease(textureView);

#ifndef __EMSCRIPTEN__
    wgpuSurfacePresent(surface_);
#endif

#if defined(WEBGPU_BACKEND_DAWN)
    wgpuDeviceTick(device_);
#elif defined(WEBGPU_BACKEND_WGPU)
    wgpuDevicePoll(device_, false, nullptr);
#endif
}

void Application::Cleanup()
{
    wgpuSurfaceUnconfigure(surface_);
    wgpuQueueRelease(queue_);
    wgpuSurfaceRelease(surface_);
    wgpuDeviceRelease(device_);
    glfwDestroyWindow(window_);
    glfwTerminate();
}

bool Application::IsRunning() const
{
    return !glfwWindowShouldClose(window_);
}

WGPUTextureView Application::GetNextSurfaceTextureView()
{
    WGPUSurfaceTexture surfaceTexture = {};
    wgpuSurfaceGetCurrentTexture(surface_, &surfaceTexture);

    if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success)
        return nullptr;

    WGPUTextureViewDescriptor textureViewDescriptor = {};
    textureViewDescriptor.nextInChain = nullptr;
    textureViewDescriptor.label = "My texture view";
    textureViewDescriptor.format = wgpuTextureGetFormat(surfaceTexture.texture);
    textureViewDescriptor.dimension = WGPUTextureViewDimension_2D;
    textureViewDescriptor.baseMipLevel = 0;
    textureViewDescriptor.mipLevelCount = 1;
    textureViewDescriptor.baseArrayLayer = 0;
    textureViewDescriptor.arrayLayerCount = 1;
    textureViewDescriptor.aspect = WGPUTextureAspect_All;

    WGPUTextureView textureView = wgpuTextureCreateView(surfaceTexture.texture, &textureViewDescriptor);

#ifndef WEBGPU_BACKEND_WGPU
    wgpuTextureRelease(surfaceTexture.texture);
#endif

    return textureView;
}
