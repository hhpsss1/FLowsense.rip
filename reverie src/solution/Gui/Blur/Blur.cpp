#include "Blur.h"

static ID3D11VertexShader* g_vertex_shader = nullptr;
static ID3D11PixelShader* g_pixel_shader_blur = nullptr;
static ID3D11Buffer* g_vertex_buffer = nullptr;
static ID3D11Buffer* g_constant_buffer = nullptr;
static ID3D11SamplerState* g_sampler_state = nullptr;
static ID3D11InputLayout* g_input_layout = nullptr;

static ID3D11PixelShader* g_pixel_shader_composite = nullptr;
static ID3D11Buffer* g_composite_constant_buffer = nullptr;
static ID3D11Buffer* g_rect_vertex_buffer = nullptr;
static ID3D11RasterizerState* g_no_cull_rasterizer_state = nullptr;
static ID3D11BlendState* g_straight_alpha_blend_state = nullptr;

static ID3D11Texture2D* g_backbuffer_copy_tex = nullptr;
static ID3D11ShaderResourceView* g_backbuffer_copy_srv = nullptr;

static ID3D11Texture2D* g_blur_texture1 = nullptr;
static ID3D11Texture2D* g_blur_texture2 = nullptr;
static ID3D11RenderTargetView* g_blur_rtv1 = nullptr;
static ID3D11RenderTargetView* g_blur_rtv2 = nullptr;
static ID3D11ShaderResourceView* g_blur_srv1 = nullptr;
static ID3D11ShaderResourceView* g_blur_srv2 = nullptr;

static ID3D11Texture2D* g_snapshot_texture = nullptr;
static ID3D11ShaderResourceView* g_snapshot_srv = nullptr;

static int g_backbuffer_width = 0;
static int g_backbuffer_height = 0;
static int g_downsampled_width = 0;
static int g_downsampled_height = 0;
static DXGI_FORMAT g_backbuffer_format = DXGI_FORMAT_UNKNOWN;

static int g_last_blur_frame = -1;
static int g_last_snapshot_blur_frame = -1;

void Blur::SaveState(ID3D11DeviceContext* ctx, CD3D11StateBackup* backup) {
    ctx->IAGetPrimitiveTopology(&backup->primitive_topology);
    ctx->IAGetVertexBuffers(0, 1, &backup->vertex_buffer, &backup->vertex_buffer_stride, &backup->vertex_buffer_offset);
    ctx->IAGetIndexBuffer(&backup->index_buffer, &backup->index_buffer_format, &backup->index_buffer_offset);
    ctx->IAGetInputLayout(&backup->input_layout);

    backup->vs_instances_count = 256;
    ctx->VSGetShader(&backup->vs, backup->vs_instances, &backup->vs_instances_count);

    backup->ps_instances_count = 256;
    ctx->PSGetShader(&backup->ps, backup->ps_instances, &backup->ps_instances_count);
    ctx->PSGetShaderResources(0, 1, &backup->ps_shader_resource);
    ctx->PSGetSamplers(0, 1, &backup->ps_sampler);

    ctx->OMGetBlendState(&backup->blend_state, backup->blend_factor, &backup->sample_mask);
    ctx->OMGetDepthStencilState(&backup->depth_stencil_state, &backup->stencil_ref);
    ctx->RSGetState(&backup->rs);

    backup->viewports_count = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ctx->RSGetViewports(&backup->viewports_count, backup->viewports);

    ctx->OMGetRenderTargets(1, &backup->rtv, &backup->dsv);
}

void Blur::RestoreState(ID3D11DeviceContext* ctx, CD3D11StateBackup* backup) {
    ctx->IASetPrimitiveTopology(backup->primitive_topology);
    ctx->IASetVertexBuffers(0, 1, &backup->vertex_buffer, &backup->vertex_buffer_stride, &backup->vertex_buffer_offset);
    ctx->IASetIndexBuffer(backup->index_buffer, backup->index_buffer_format, backup->index_buffer_offset);
    ctx->IASetInputLayout(backup->input_layout);

    ctx->VSSetShader(backup->vs, backup->vs_instances, backup->vs_instances_count);
    ctx->PSSetShader(backup->ps, backup->ps_instances, backup->ps_instances_count);
    ctx->PSSetShaderResources(0, 1, &backup->ps_shader_resource);
    ctx->PSSetSamplers(0, 1, &backup->ps_sampler);

    ctx->OMSetBlendState(backup->blend_state, backup->blend_factor, backup->sample_mask);
    ctx->OMSetDepthStencilState(backup->depth_stencil_state, backup->stencil_ref);
    ctx->RSSetState(backup->rs);
    ctx->RSSetViewports(backup->viewports_count, backup->viewports);
    ctx->OMSetRenderTargets(1, &backup->rtv, backup->dsv);
}

bool Blur::EnsureResources(ID3D11Device* device) {
    if (g_vertex_shader && g_pixel_shader_blur && g_constant_buffer && g_input_layout)
        return true;

    const char* vs_code =
        "struct VS_INPUT { float2 pos : POSITION; float2 uv : TEXCOORD; };"
        "struct PS_INPUT { float4 pos : SV_POSITION; float2 uv : TEXCOORD; };"
        "PS_INPUT main(VS_INPUT input) {"
        "    PS_INPUT output;"
        "    output.pos = float4(input.pos, 0.0, 1.0);"
        "    output.uv = input.uv;"
        "    return output;"
        "}";

    ID3DBlob* vs_blob = nullptr;
    HRESULT hr = D3DCompile(vs_code, strlen(vs_code), nullptr, nullptr, nullptr, "main", "vs_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &vs_blob, nullptr);
    if (FAILED(hr))
        return false;

    hr = device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &g_vertex_shader);
    if (FAILED(hr)) {
        vs_blob->Release();
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layout_desc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = device->CreateInputLayout(layout_desc, 2, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &g_input_layout);
    vs_blob->Release();
    if (FAILED(hr))
        return false;

    float vertices[] = {
        -1.0f,  1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 1.0f
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = vertices;
    hr = device->CreateBuffer(&bd, &sd, &g_vertex_buffer);
    if (FAILED(hr))
        return false;

    static const char* ps_code_blur = R"(
    Texture2D tex      : register(t0);
    SamplerState samp  : register(s0);

    cbuffer BlurParams : register(b0)
    {
        float2 texelSize;
        float2 direction;
    };

    static const float weights[9] = {
        0.05, 0.09, 0.12, 0.15, 0.18, 0.15, 0.12, 0.09, 0.05
    };

    float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD) : SV_Target
    {
        if (direction.x == 0.0 && direction.y == 0.0)
            return tex.Sample(samp, uv);

        float4 color = float4(0.0, 0.0, 0.0, 0.0);

        [unroll]
        for (int i = -4; i <= 4; ++i)
        {
            float w = weights[i + 4];
            float2 off = direction * (float(i) * texelSize);
            color += tex.Sample(samp, uv + off) * w;
        }

        color.a = 1.0;
        return color;
    }
)";

    ID3DBlob* ps_blob_blur = nullptr;
    hr = D3DCompile(ps_code_blur, strlen(ps_code_blur), nullptr, nullptr, nullptr, "main", "ps_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &ps_blob_blur, nullptr);
    if (FAILED(hr))
        return false;

    hr = device->CreatePixelShader(ps_blob_blur->GetBufferPointer(), ps_blob_blur->GetBufferSize(), nullptr, &g_pixel_shader_blur);
    ps_blob_blur->Release();
    if (FAILED(hr))
        return false;

    bd = {};
    bd.ByteWidth = sizeof(float) * 4;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = device->CreateBuffer(&bd, nullptr, &g_constant_buffer);
    if (FAILED(hr))
        return false;

    D3D11_SAMPLER_DESC samp_desc = {};
    samp_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samp_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samp_desc.MinLOD = 0;
    samp_desc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = device->CreateSamplerState(&samp_desc, &g_sampler_state);
    if (FAILED(hr))
        return false;

    return true;
}

bool Blur::EnsureTextures(ID3D11Device* device, int width, int height, DXGI_FORMAT format) {
    if (width == g_backbuffer_width && height == g_backbuffer_height && format == g_backbuffer_format)
        return true;

    ReleaseBlurResources();

    g_backbuffer_width = width;
    g_backbuffer_height = height;
    g_backbuffer_format = format;

    g_downsampled_width = (std::max)(1, width / BLUR_DOWNSAMPLE_FACTOR);
    g_downsampled_height = (std::max)(1, height / BLUR_DOWNSAMPLE_FACTOR);

    D3D11_TEXTURE2D_DESC copy_desc = {};
    copy_desc.Width = width;
    copy_desc.Height = height;
    copy_desc.MipLevels = 1;
    copy_desc.ArraySize = 1;
    copy_desc.Format = format;
    copy_desc.SampleDesc.Count = 1;
    copy_desc.Usage = D3D11_USAGE_DEFAULT;
    copy_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    if (FAILED(device->CreateTexture2D(&copy_desc, nullptr, &g_backbuffer_copy_tex)))
        return false;

    if (FAILED(device->CreateShaderResourceView(g_backbuffer_copy_tex, nullptr, &g_backbuffer_copy_srv)))
        return false;

    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = g_downsampled_width;
    tex_desc.Height = g_downsampled_height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Format = format;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    if (FAILED(device->CreateTexture2D(&tex_desc, nullptr, &g_blur_texture1)))
        return false;

    if (FAILED(device->CreateTexture2D(&tex_desc, nullptr, &g_blur_texture2)))
        return false;

    if (FAILED(device->CreateRenderTargetView(g_blur_texture1, nullptr, &g_blur_rtv1)))
        return false;

    if (FAILED(device->CreateRenderTargetView(g_blur_texture2, nullptr, &g_blur_rtv2)))
        return false;

    if (FAILED(device->CreateShaderResourceView(g_blur_texture1, nullptr, &g_blur_srv1)))
        return false;

    if (FAILED(device->CreateShaderResourceView(g_blur_texture2, nullptr, &g_blur_srv2)))
        return false;

    return true;
}

bool Blur::EnsureCompositeResources(ID3D11Device* device) {
    if (g_pixel_shader_composite && g_composite_constant_buffer && g_rect_vertex_buffer && g_no_cull_rasterizer_state && g_straight_alpha_blend_state)
        return true;

    if (!g_pixel_shader_composite) {
        static const char* ps_code_composite = R"(
        Texture2D tex      : register(t0);
        SamplerState samp  : register(s0);

        cbuffer CompositeParams : register(b0)
        {
            float4 rect;
            float4 rounding_alpha_pad;
        };

        float rounded_box_sdf(float2 p, float2 half_size, float r)
        {
            float2 d = abs(p) - half_size + r;
            return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - r;
        }

        float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD) : SV_Target
        {
            float2 center = (rect.xy + rect.zw) * 0.5;
            float2 half_size = (rect.zw - rect.xy) * 0.5;
            float2 p = pos.xy - center;
            float d = rounded_box_sdf(p, half_size, rounding_alpha_pad.x);
            float mask = 1.0 - smoothstep(0.0, 1.5, d);
            float4 color = tex.Sample(samp, uv);
            color.a = mask * rounding_alpha_pad.y;
            return color;
        }
    )";

        ID3DBlob* ps_blob = nullptr;
        HRESULT hr = D3DCompile(ps_code_composite, strlen(ps_code_composite), nullptr, nullptr, nullptr, "main", "ps_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &ps_blob, nullptr);
        if (FAILED(hr))
            return false;

        hr = device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &g_pixel_shader_composite);
        ps_blob->Release();
        if (FAILED(hr))
            return false;
    }

    if (!g_composite_constant_buffer) {
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = sizeof(float) * 8;
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        if (FAILED(device->CreateBuffer(&bd, nullptr, &g_composite_constant_buffer)))
            return false;
    }

    if (!g_rect_vertex_buffer) {
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(float) * 4 * 4;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        if (FAILED(device->CreateBuffer(&bd, nullptr, &g_rect_vertex_buffer)))
            return false;
    }

    if (!g_no_cull_rasterizer_state) {
        D3D11_RASTERIZER_DESC rd = {};
        rd.FillMode = D3D11_FILL_SOLID;
        rd.CullMode = D3D11_CULL_NONE;
        rd.ScissorEnable = FALSE;
        rd.DepthClipEnable = TRUE;

        if (FAILED(device->CreateRasterizerState(&rd, &g_no_cull_rasterizer_state)))
            return false;
    }

    if (!g_straight_alpha_blend_state) {
        D3D11_BLEND_DESC bd = {};
        bd.RenderTarget[0].BlendEnable = TRUE;
        bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        if (FAILED(device->CreateBlendState(&bd, &g_straight_alpha_blend_state)))
            return false;
    }

    return true;
}

void Blur::RenderFullBlur(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
    CBlurCallbackData* data = (CBlurCallbackData*)cmd->UserCallbackData;
    ID3D11Device* device = data->device;
    ID3D11DeviceContext* ctx = data->ctx;

    SaveState(ctx, &data->state);

    if (!EnsureResources(device)) {
        RestoreState(ctx, &data->state);
        delete data;
        return;
    }

    ID3D11RenderTargetView* rtv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, nullptr);

    if (rtv) {
        ID3D11Resource* backbuffer_resource = nullptr;
        rtv->GetResource(&backbuffer_resource);

        if (backbuffer_resource) {
            ID3D11Texture2D* backbuffer_texture = nullptr;
            if (SUCCEEDED(backbuffer_resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&backbuffer_texture))) {
                D3D11_TEXTURE2D_DESC desc;
                backbuffer_texture->GetDesc(&desc);

                if (EnsureTextures(device, desc.Width, desc.Height, desc.Format))
                    ctx->CopyResource(g_backbuffer_copy_tex, backbuffer_resource);

                backbuffer_texture->Release();
            }
            backbuffer_resource->Release();
        }
        rtv->Release();
    }

    if (!g_pixel_shader_blur || !g_blur_rtv1 || !g_blur_rtv2 || !g_backbuffer_copy_srv) {
        RestoreState(ctx, &data->state);
        delete data;
        return;
    }

    D3D11_VIEWPORT vp = {};
    vp.Width = (float)g_downsampled_width;
    vp.Height = (float)g_downsampled_height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vp);

    ctx->VSSetShader(g_vertex_shader, nullptr, 0);
    ctx->IASetInputLayout(g_input_layout);

    UINT stride = sizeof(float) * 4;
    UINT offset = 0;
    ctx->IASetVertexBuffers(0, 1, &g_vertex_buffer, &stride, &offset);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    ctx->PSSetSamplers(0, 1, &g_sampler_state);
    ctx->PSSetShader(g_pixel_shader_blur, nullptr, 0);

    ID3D11RenderTargetView* null_rtv = nullptr;
    ID3D11ShaderResourceView* null_srv = nullptr;

    float constants_downsample[4] = { 1.0f / static_cast<float>(g_backbuffer_width), 1.0f / static_cast<float>(g_backbuffer_height), 0.0f, 0.0f };
    ctx->UpdateSubresource(g_constant_buffer, 0, nullptr, constants_downsample, 0, 0);
    ctx->PSSetConstantBuffers(0, 1, &g_constant_buffer);

    ctx->OMSetRenderTargets(1, &null_rtv, nullptr);
    ctx->PSSetShaderResources(0, 1, &g_backbuffer_copy_srv);
    ctx->OMSetRenderTargets(1, &g_blur_rtv1, nullptr);
    ctx->Draw(4, 0);

    float constants_h[4] = { 1.0f / static_cast<float>(g_downsampled_width), 1.0f / static_cast<float>(g_downsampled_height), 1.0f, 0.0f };
    ctx->UpdateSubresource(g_constant_buffer, 0, nullptr, constants_h, 0, 0);

    ctx->OMSetRenderTargets(1, &null_rtv, nullptr);
    ctx->PSSetShaderResources(0, 1, &g_blur_srv1);
    ctx->OMSetRenderTargets(1, &g_blur_rtv2, nullptr);
    ctx->Draw(4, 0);

    float constants_v[4] = { 1.0f / static_cast<float>(g_downsampled_width), 1.0f / static_cast<float>(g_downsampled_height), 0.0f, 1.0f };
    ctx->UpdateSubresource(g_constant_buffer, 0, nullptr, constants_v, 0, 0);

    ctx->OMSetRenderTargets(1, &null_rtv, nullptr);
    ctx->PSSetShaderResources(0, 1, &g_blur_srv2);
    ctx->OMSetRenderTargets(1, &g_blur_rtv1, nullptr);
    ctx->Draw(4, 0);

    ctx->PSSetShaderResources(0, 1, &null_srv);

    RestoreState(ctx, &data->state);
    delete data;
}

void Blur::RenderFullBlurFromSnapshot(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
    CBlurCallbackData* data = (CBlurCallbackData*)cmd->UserCallbackData;
    ID3D11Device* device = data->device;
    ID3D11DeviceContext* ctx = data->ctx;

    SaveState(ctx, &data->state);

    if (!EnsureResources(device) || !g_snapshot_texture || !g_blur_texture1) {
        RestoreState(ctx, &data->state);
        delete data;
        return;
    }

    ctx->CopyResource(g_blur_texture1, g_snapshot_texture);

    D3D11_VIEWPORT vp = {};
    vp.Width = (float)g_downsampled_width;
    vp.Height = (float)g_downsampled_height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vp);

    ctx->VSSetShader(g_vertex_shader, nullptr, 0);
    ctx->IASetInputLayout(g_input_layout);

    UINT stride = sizeof(float) * 4;
    UINT offset = 0;
    ctx->IASetVertexBuffers(0, 1, &g_vertex_buffer, &stride, &offset);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    ctx->PSSetSamplers(0, 1, &g_sampler_state);
    ctx->PSSetShader(g_pixel_shader_blur, nullptr, 0);

    ID3D11RenderTargetView* null_rtv = nullptr;
    ID3D11ShaderResourceView* null_srv = nullptr;

    float constants_h[4] = { 1.0f / static_cast<float>(g_downsampled_width), 1.0f / static_cast<float>(g_downsampled_height), 1.0f, 0.0f };
    ctx->UpdateSubresource(g_constant_buffer, 0, nullptr, constants_h, 0, 0);
    ctx->PSSetConstantBuffers(0, 1, &g_constant_buffer);

    ctx->OMSetRenderTargets(1, &null_rtv, nullptr);
    ctx->PSSetShaderResources(0, 1, &g_blur_srv1);
    ctx->OMSetRenderTargets(1, &g_blur_rtv2, nullptr);
    ctx->Draw(4, 0);

    float constants_v[4] = { 1.0f / static_cast<float>(g_downsampled_width), 1.0f / static_cast<float>(g_downsampled_height), 0.0f, 1.0f };
    ctx->UpdateSubresource(g_constant_buffer, 0, nullptr, constants_v, 0, 0);

    ctx->OMSetRenderTargets(1, &null_rtv, nullptr);
    ctx->PSSetShaderResources(0, 1, &g_blur_srv2);
    ctx->OMSetRenderTargets(1, &g_blur_rtv1, nullptr);
    ctx->Draw(4, 0);

    ctx->PSSetShaderResources(0, 1, &null_srv);

    RestoreState(ctx, &data->state);
    delete data;
}

void Blur::RenderPostBlur(ID3D11Device* device, ID3D11DeviceContext* ctx, const ImVec2& min, const ImVec2& max, float rounding, float alpha) {
    if (alpha <= 0.001f)
        return;

    if (!EnsureResources(device) || !EnsureCompositeResources(device))
        return;

    CD3D11StateBackup backup;
    SaveState(ctx, &backup);

    ID3D11RenderTargetView* rtv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, nullptr);
    if (!rtv) {
        RestoreState(ctx, &backup);
        return;
    }

    ID3D11Resource* backbuffer_resource = nullptr;
    rtv->GetResource(&backbuffer_resource);

    if (backbuffer_resource) {
        ID3D11Texture2D* backbuffer_texture = nullptr;
        if (SUCCEEDED(backbuffer_resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&backbuffer_texture))) {
            D3D11_TEXTURE2D_DESC desc;
            backbuffer_texture->GetDesc(&desc);

            if (EnsureTextures(device, desc.Width, desc.Height, desc.Format))
                ctx->CopyResource(g_backbuffer_copy_tex, backbuffer_resource);

            backbuffer_texture->Release();
        }
        backbuffer_resource->Release();
    }

    if (!g_pixel_shader_blur || !g_blur_rtv1 || !g_blur_rtv2 || !g_backbuffer_copy_srv) {
        rtv->Release();
        RestoreState(ctx, &backup);
        return;
    }

    D3D11_VIEWPORT vp_down = {};
    vp_down.Width = (float)g_downsampled_width;
    vp_down.Height = (float)g_downsampled_height;
    vp_down.MinDepth = 0.0f;
    vp_down.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vp_down);

    ctx->RSSetState(g_no_cull_rasterizer_state);
    ctx->VSSetShader(g_vertex_shader, nullptr, 0);
    ctx->IASetInputLayout(g_input_layout);

    UINT stride = sizeof(float) * 4;
    UINT offset = 0;
    ctx->IASetVertexBuffers(0, 1, &g_vertex_buffer, &stride, &offset);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    ctx->PSSetSamplers(0, 1, &g_sampler_state);
    ctx->PSSetShader(g_pixel_shader_blur, nullptr, 0);

    ID3D11RenderTargetView* null_rtv = nullptr;
    ID3D11ShaderResourceView* null_srv = nullptr;

    float constants_downsample[4] = { 1.0f / static_cast<float>(g_backbuffer_width), 1.0f / static_cast<float>(g_backbuffer_height), 0.0f, 0.0f };
    ctx->UpdateSubresource(g_constant_buffer, 0, nullptr, constants_downsample, 0, 0);
    ctx->PSSetConstantBuffers(0, 1, &g_constant_buffer);

    ctx->OMSetRenderTargets(1, &null_rtv, nullptr);
    ctx->PSSetShaderResources(0, 1, &g_backbuffer_copy_srv);
    ctx->OMSetRenderTargets(1, &g_blur_rtv1, nullptr);
    ctx->Draw(4, 0);

    float constants_h[4] = { 1.0f / static_cast<float>(g_downsampled_width), 1.0f / static_cast<float>(g_downsampled_height), 1.0f, 0.0f };
    ctx->UpdateSubresource(g_constant_buffer, 0, nullptr, constants_h, 0, 0);

    ctx->OMSetRenderTargets(1, &null_rtv, nullptr);
    ctx->PSSetShaderResources(0, 1, &g_blur_srv1);
    ctx->OMSetRenderTargets(1, &g_blur_rtv2, nullptr);
    ctx->Draw(4, 0);

    float constants_v[4] = { 1.0f / static_cast<float>(g_downsampled_width), 1.0f / static_cast<float>(g_downsampled_height), 0.0f, 1.0f };
    ctx->UpdateSubresource(g_constant_buffer, 0, nullptr, constants_v, 0, 0);

    ctx->OMSetRenderTargets(1, &null_rtv, nullptr);
    ctx->PSSetShaderResources(0, 1, &g_blur_srv2);
    ctx->OMSetRenderTargets(1, &g_blur_rtv1, nullptr);
    ctx->Draw(4, 0);

    const float pad = 3.0f;
    const float px0 = min.x - pad;
    const float py0 = min.y - pad;
    const float px1 = max.x + pad;
    const float py1 = max.y + pad;

    const float w = (float)g_backbuffer_width;
    const float h = (float)g_backbuffer_height;

    const float x0 = (px0 / w) * 2.0f - 1.0f;
    const float y0 = 1.0f - (py0 / h) * 2.0f;
    const float x1 = (px1 / w) * 2.0f - 1.0f;
    const float y1 = 1.0f - (py1 / h) * 2.0f;

    const float u0 = px0 / w;
    const float v0 = py0 / h;
    const float u1 = px1 / w;
    const float v1 = py1 / h;

    float rect_vertices[16] = {
        x0, y0, u0, v0,
        x1, y0, u1, v0,
        x0, y1, u0, v1,
        x1, y1, u1, v1
    };
    ctx->UpdateSubresource(g_rect_vertex_buffer, 0, nullptr, rect_vertices, 0, 0);

    float composite_constants[8] = { min.x, min.y, max.x, max.y, rounding, alpha, 0.0f, 0.0f };
    ctx->UpdateSubresource(g_composite_constant_buffer, 0, nullptr, composite_constants, 0, 0);

    D3D11_VIEWPORT vp_full = {};
    vp_full.Width = w;
    vp_full.Height = h;
    vp_full.MinDepth = 0.0f;
    vp_full.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vp_full);

    ctx->IASetVertexBuffers(0, 1, &g_rect_vertex_buffer, &stride, &offset);
    ctx->PSSetShader(g_pixel_shader_composite, nullptr, 0);
    ctx->PSSetConstantBuffers(0, 1, &g_composite_constant_buffer);

    const float blend_factor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    ctx->OMSetBlendState(g_straight_alpha_blend_state, blend_factor, 0xffffffff);

    ctx->OMSetRenderTargets(1, &null_rtv, nullptr);
    ctx->PSSetShaderResources(0, 1, &g_blur_srv1);
    ctx->OMSetRenderTargets(1, &rtv, nullptr);
    ctx->Draw(4, 0);

    ctx->PSSetShaderResources(0, 1, &null_srv);

    rtv->Release();
    RestoreState(ctx, &backup);
}

void Blur::CapturePostFrame(ID3D11Device* device, ID3D11DeviceContext* ctx) {
    ID3D11RenderTargetView* rtv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, nullptr);
    if (!rtv)
        return;

    ID3D11Resource* bb_res = nullptr;
    rtv->GetResource(&bb_res);
    rtv->Release();
    if (!bb_res)
        return;

    ID3D11Texture2D* bb_tex = nullptr;
    if (FAILED(bb_res->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&bb_tex))) {
        bb_res->Release();
        return;
    }

    D3D11_TEXTURE2D_DESC desc;
    bb_tex->GetDesc(&desc);
    bb_tex->Release();

    int down_w = (std::max)(1, (int)desc.Width / BLUR_DOWNSAMPLE_FACTOR);
    int down_h = (std::max)(1, (int)desc.Height / BLUR_DOWNSAMPLE_FACTOR);

    if (!g_snapshot_texture) {
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = down_w;
        td.Height = down_h;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = desc.Format;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        if (FAILED(device->CreateTexture2D(&td, nullptr, &g_snapshot_texture))) {
            bb_res->Release();
            return;
        }

        if (FAILED(device->CreateShaderResourceView(g_snapshot_texture, nullptr, &g_snapshot_srv))) {
            g_snapshot_texture->Release();
            g_snapshot_texture = nullptr;
            bb_res->Release();
            return;
        }
    }

    ctx->CopySubresourceRegion(g_snapshot_texture, 0, 0, 0, 0, bb_res, 0, nullptr);
    bb_res->Release();
}

void Blur::DrawRectBlur(ImDrawList* draw_list, ID3D11Device* device, ID3D11DeviceContext* ctx, const ImVec2& min, const ImVec2& max, float rounding) {
    int current_frame = ImGui::GetFrameCount();

    if (g_last_blur_frame != current_frame) {
        g_last_blur_frame = current_frame;

        CBlurCallbackData* data = new CBlurCallbackData{ device, ctx, {}, 0 };
        draw_list->AddCallback(RenderFullBlur, data);
    }

    if (g_blur_srv1) {
        const float alpha = ImGui::GetStyle().Alpha;
        const ImU32 color = IM_COL32(255, 255, 255, static_cast<int>(255 * alpha));

        ImVec2 display_size = ImGui::GetIO().DisplaySize;
        ImVec2 uv0 = ImVec2(min.x / display_size.x, min.y / display_size.y);
        ImVec2 uv1 = ImVec2(max.x / display_size.x, max.y / display_size.y);

        draw_list->AddImageRounded((ImTextureID)g_blur_srv1, min, max, uv0, uv1, color, rounding, ImDrawFlags_RoundCornersAll);
    }
}

void Blur::DrawBackgroundBlur(ImDrawList* draw_list, ID3D11Device* device, ID3D11DeviceContext* ctx, float rounding) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->Hidden || window->SkipItems || ImGui::GetStyle().Alpha < 0.01f)
        return;

    ImVec2 window_pos = ImGui::GetWindowPos();
    ImVec2 window_size = ImGui::GetWindowSize();
    DrawRectBlur(draw_list, device, ctx, window_pos, window_pos + window_size, rounding);
}

void Blur::DrawRectBlurFromSnapshot(ImDrawList* draw_list, ID3D11Device* device, ID3D11DeviceContext* ctx, const ImVec2& min, const ImVec2& max, float rounding) {
    if (!g_snapshot_texture)
        return;

    int current_frame = ImGui::GetFrameCount();

    if (g_last_snapshot_blur_frame != current_frame) {
        g_last_snapshot_blur_frame = current_frame;

        CBlurCallbackData* bdata = new CBlurCallbackData{ device, ctx, {}, 0 };
        draw_list->AddCallback(RenderFullBlurFromSnapshot, bdata);
    }

    if (g_blur_srv1) {
        const float alpha = ImGui::GetStyle().Alpha;
        ImVec2 display_size = ImGui::GetIO().DisplaySize;
        ImVec2 uv0(min.x / display_size.x, min.y / display_size.y);
        ImVec2 uv1(max.x / display_size.x, max.y / display_size.y);
        draw_list->AddImageRounded((ImTextureID)g_blur_srv1, min, max, uv0, uv1, IM_COL32(255, 255, 255, (int)(255 * alpha)), rounding, ImDrawFlags_RoundCornersAll);
    }
}

void Blur::DrawBackgroundBlurFromSnapshot(ImDrawList* draw_list, ID3D11Device* device, ID3D11DeviceContext* ctx, float rounding) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->Hidden || window->SkipItems || ImGui::GetStyle().Alpha < 0.01f)
        return;

    ImVec2 wp = ImGui::GetWindowPos();
    ImVec2 ws = ImGui::GetWindowSize();
    DrawRectBlurFromSnapshot(draw_list, device, ctx, wp, wp + ws, rounding);
}

void Blur::ReleaseBlurResources() {
    if (g_vertex_shader) {
        g_vertex_shader->Release();
        g_vertex_shader = nullptr;
    }

    if (g_pixel_shader_blur) {
        g_pixel_shader_blur->Release();
        g_pixel_shader_blur = nullptr;
    }

    if (g_vertex_buffer) {
        g_vertex_buffer->Release();
        g_vertex_buffer = nullptr;
    }

    if (g_constant_buffer) {
        g_constant_buffer->Release();
        g_constant_buffer = nullptr;
    }

    if (g_sampler_state) {
        g_sampler_state->Release();
        g_sampler_state = nullptr;
    }

    if (g_input_layout) {
        g_input_layout->Release();
        g_input_layout = nullptr;
    }

    if (g_pixel_shader_composite) {
        g_pixel_shader_composite->Release();
        g_pixel_shader_composite = nullptr;
    }

    if (g_composite_constant_buffer) {
        g_composite_constant_buffer->Release();
        g_composite_constant_buffer = nullptr;
    }

    if (g_rect_vertex_buffer) {
        g_rect_vertex_buffer->Release();
        g_rect_vertex_buffer = nullptr;
    }

    if (g_no_cull_rasterizer_state) {
        g_no_cull_rasterizer_state->Release();
        g_no_cull_rasterizer_state = nullptr;
    }

    if (g_straight_alpha_blend_state) {
        g_straight_alpha_blend_state->Release();
        g_straight_alpha_blend_state = nullptr;
    }

    if (g_backbuffer_copy_srv) {
        g_backbuffer_copy_srv->Release();
        g_backbuffer_copy_srv = nullptr;
    }

    if (g_backbuffer_copy_tex) {
        g_backbuffer_copy_tex->Release();
        g_backbuffer_copy_tex = nullptr;
    }

    if (g_blur_texture1) {
        g_blur_texture1->Release();
        g_blur_texture1 = nullptr;
    }

    if (g_blur_texture2) {
        g_blur_texture2->Release();
        g_blur_texture2 = nullptr;
    }

    if (g_blur_rtv1) {
        g_blur_rtv1->Release();
        g_blur_rtv1 = nullptr;
    }

    if (g_blur_rtv2) {
        g_blur_rtv2->Release();
        g_blur_rtv2 = nullptr;
    }

    if (g_blur_srv1) {
        g_blur_srv1->Release();
        g_blur_srv1 = nullptr;
    }

    if (g_blur_srv2) {
        g_blur_srv2->Release();
        g_blur_srv2 = nullptr;
    }

    if (g_snapshot_srv) {
        g_snapshot_srv->Release();
        g_snapshot_srv = nullptr;
    }

    if (g_snapshot_texture) {
        g_snapshot_texture->Release();
        g_snapshot_texture = nullptr;
    }

    g_backbuffer_width = 0;
    g_backbuffer_height = 0;
    g_downsampled_width = 0;
    g_downsampled_height = 0;
    g_backbuffer_format = DXGI_FORMAT_UNKNOWN;
    g_last_blur_frame = -1;
    g_last_snapshot_blur_frame = -1;
}