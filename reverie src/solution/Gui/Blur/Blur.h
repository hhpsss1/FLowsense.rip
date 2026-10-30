#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <D3Dcompiler.h>
#include <d3d11.h>

#define BLUR_DOWNSAMPLE_FACTOR 2

struct CD3D11StateBackup {
    ID3D11RasterizerState* rs = nullptr;
    ID3D11BlendState* blend_state = nullptr;
    FLOAT blend_factor[4] = { 0 };
    UINT sample_mask = 0;
    ID3D11DepthStencilState* depth_stencil_state = nullptr;
    UINT stencil_ref = 0;
    ID3D11ShaderResourceView* ps_shader_resource = nullptr;
    ID3D11SamplerState* ps_sampler = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11VertexShader* vs = nullptr;
    UINT ps_instances_count = 0, vs_instances_count = 0;
    ID3D11ClassInstance* ps_instances[256] = { nullptr }, * vs_instances[256] = { nullptr };
    D3D11_PRIMITIVE_TOPOLOGY primitive_topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ID3D11Buffer* index_buffer = nullptr, * vertex_buffer = nullptr;
    UINT index_buffer_offset = 0, vertex_buffer_stride = 0, vertex_buffer_offset = 0;
    DXGI_FORMAT index_buffer_format = DXGI_FORMAT_UNKNOWN;
    ID3D11InputLayout* input_layout = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11DepthStencilView* dsv = nullptr;
    D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = { {0} };
    UINT viewports_count = 0;

    ~CD3D11StateBackup() {
        if (this->rs)
            this->rs->Release();

        if (this->blend_state)
            this->blend_state->Release();

        if (this->depth_stencil_state)
            this->depth_stencil_state->Release();

        if (this->ps_shader_resource)
            this->ps_shader_resource->Release();

        if (this->ps_sampler)
            this->ps_sampler->Release();

        if (this->ps)
            this->ps->Release();

        if (this->vs)
            this->vs->Release();

        for (UINT i = 0; i < this->ps_instances_count; i++)
            if (this->ps_instances[i])
                this->ps_instances[i]->Release();

        for (UINT i = 0; i < this->vs_instances_count; i++)
            if (this->vs_instances[i])
                this->vs_instances[i]->Release();

        if (this->index_buffer)
            this->index_buffer->Release();

        if (this->vertex_buffer)
            this->vertex_buffer->Release();

        if (this->input_layout)
            this->input_layout->Release();

        if (this->rtv)
            this->rtv->Release();

        if (this->dsv)
            this->dsv->Release();
    }
};

struct CBlurCallbackData {
    ID3D11Device* device;
    ID3D11DeviceContext* ctx;
    CD3D11StateBackup state;
    int pass_count = 0;
};

namespace Blur {
    void SaveState(ID3D11DeviceContext* ctx, CD3D11StateBackup* backup);
    void RestoreState(ID3D11DeviceContext* ctx, CD3D11StateBackup* backup);
    bool EnsureResources(ID3D11Device* device);
    bool EnsureTextures(ID3D11Device* device, int width, int height, DXGI_FORMAT format);
    bool EnsureCompositeResources(ID3D11Device* device);
    void RenderFullBlur(const ImDrawList* parent_list, const ImDrawCmd* cmd);
    void RenderFullBlurFromSnapshot(const ImDrawList* parent_list, const ImDrawCmd* cmd);
    void RenderPostBlur(ID3D11Device* device, ID3D11DeviceContext* ctx, const ImVec2& min, const ImVec2& max, float rounding, float alpha);
    void CapturePostFrame(ID3D11Device* device, ID3D11DeviceContext* ctx);
    void DrawRectBlur(ImDrawList* draw_list, ID3D11Device* device, ID3D11DeviceContext* ctx, const ImVec2& min, const ImVec2& max, float rounding = 0);
    void DrawBackgroundBlur(ImDrawList* draw_list, ID3D11Device* device, ID3D11DeviceContext* ctx, float rounding = 0);
    void DrawRectBlurFromSnapshot(ImDrawList* draw_list, ID3D11Device* device, ID3D11DeviceContext* ctx, const ImVec2& min, const ImVec2& max, float rounding = 0);
    void DrawBackgroundBlurFromSnapshot(ImDrawList* draw_list, ID3D11Device* device, ID3D11DeviceContext* ctx, float rounding = 0);
    void ReleaseBlurResources();
}