#include <shared.h>

CImageProxySource* CImageResourceManager::LoadImageFromUrl(void* panel, void* default_resource_url, const char* url, uint32_t format, CRenderViewport* image_format_out) {
    return Memory->Vtable.Execute<CImageProxySource*>(this, 0, panel, default_resource_url, url, format, image_format_out);
}

CImageProxySource* CImageResourceManager::LoadImageInternal(const char* image_path, uint32_t format) {
    CRenderViewport image_data{};
    image_data.image_path = image_path;

    return this->LoadImageFromUrl(nullptr, nullptr, image_path, format, &image_data);
}

ID3D11ShaderResourceView* CImageProxySource::GetNativeTexture() {
    if (!this)
        return nullptr;

    auto texture = this->GetTextureID();
    if (!texture)
        return nullptr;

    auto data = texture->data;
    if (!data)
        return nullptr;

    auto dx11texture = data->dx11texture;
    if (!dx11texture)
        return nullptr;

    return dx11texture->srv0;
}

ImVec2 CImageProxySource::GetImageSize() {
    if (!this)
        return ImVec2(0, 0);

    auto texture = this->GetTextureID();
    if (!texture)
        return ImVec2(0, 0);

    return ImVec2(float(texture->width), float(texture->height));
}