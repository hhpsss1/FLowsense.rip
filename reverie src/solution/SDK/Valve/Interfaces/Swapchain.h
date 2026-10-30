#pragma once

struct CSwapChainBase {
    virtual ~CSwapChainBase() = 0;

    virtual int GetScreenWidth() = 0;
    virtual int GetScreenHeight() = 0;
};

struct CSwapChain : CSwapChainBase {
    char pad000[0x8];
    HWND window;
    char pad001[0x8];
    int width;
    int height;
    char pad003[0x148];
    IDXGISwapChain* dxgi_swapchain;
    char pad004[0x90];
};