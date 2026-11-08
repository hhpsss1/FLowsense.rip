#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "Colors.h"
#include "Variables.h"
#include "Elements.h"
#include <vector>
#include <sstream>
#include <string>
#include <functional>
#include <fstream>
#include <d3d11.h>

struct Vector;

using namespace ImGui;

namespace OtcStyle {
    inline ImU32 Pack(const ImVec4& c, float a = 1.f) {
        return IM_COL32(
            static_cast<int>(c.x * 255.f),
            static_cast<int>(c.y * 255.f),
            static_cast<int>(c.z * 255.f),
            static_cast<int>(c.w * 255.f * a));
    }

    inline ImU32 Accent(float a = 1.f) { return IM_COL32(181, 0, 41, static_cast<int>(255.f * a)); }
    inline ImU32 AccentShade(float a = 1.f) { return IM_COL32(156, 0, 16, static_cast<int>(255.f * a)); }
    inline ImU32 Text(float a = 1.f) { return IM_COL32(206, 206, 206, static_cast<int>(255.f * a)); }
    inline ImU32 TextDim(float a = 1.f) { return IM_COL32(153, 153, 153, static_cast<int>(255.f * a)); }
    inline ImU32 Control(float a = 1.f) { return IM_COL32(36, 36, 36, static_cast<int>(190.f * a)); }
    inline ImU32 Border(float a = 1.f) { return IM_COL32(0, 0, 0, static_cast<int>(255.f * a)); }
    inline ImU32 Track(float a = 1.f) { return IM_COL32(69, 69, 69, static_cast<int>(190.f * a)); }
    inline ImU32 CheckOff(float a = 1.f) { return IM_COL32(69, 69, 69, static_cast<int>(255.f * a)); }
    inline ImU32 GroupFace(float a = 1.f) { return IM_COL32(12, 12, 12, static_cast<int>(72.f * a)); }
    inline ImU32 GroupLine(float a = 1.f) { return IM_COL32(44, 44, 44, static_cast<int>(180.f * a)); }
    inline ImU32 WindowFace(float a = 1.f) { return IM_COL32(4, 4, 4, static_cast<int>(175.f * a)); }
    inline ImU32 WindowLine(float a = 1.f) { return IM_COL32(57, 57, 57, static_cast<int>(210.f * a)); }
    inline ImU32 WindowBetween(float a = 1.f) { return IM_COL32(35, 35, 35, static_cast<int>(150.f * a)); }
    inline ImU32 WindowShadow(float a = 1.f) { return IM_COL32(0, 0, 0, static_cast<int>(150.f * a)); }
    inline ImU32 PanelFace(float a = 1.f) { return IM_COL32(9, 9, 9, static_cast<int>(82.f * a)); }
    inline ImU32 PanelLine(float a = 1.f) { return IM_COL32(44, 44, 44, static_cast<int>(145.f * a)); }
    inline ImU32 PanelInner(float a = 1.f) { return IM_COL32(9, 9, 9, static_cast<int>(120.f * a)); }
    inline ImU32 TabIdle(float a = 1.f) { return IM_COL32(110, 110, 115, static_cast<int>(255.f * a)); }
    inline ImU32 TabIdleBottom(float a = 1.f) { return IM_COL32(50, 50, 55, static_cast<int>(255.f * a)); }
}

struct OtcPickerState {
    ImVec2 marker{ -1.f, -1.f };
    bool held{ false };
    bool held_alpha{ false };
};

bool DrawOtcColorPicker(ImDrawList* dl, const ImVec2& popup_min, Color_t& col, OtcPickerState& state);

enum class EBindType : int {
    hold = 0,
    toggle = 1
};

namespace Binds {
    enum class EEntryKind { checkbox, slider_int, slider_float, dropdown, multi };

    struct Entry {
        uint32_t uid{};
        std::string label;
        std::string group;
        EEntryKind kind{ EEntryKind::checkbox };
        int* key{};
        int* type{};
        bool* enabled{};
        bool* visible{};
        bool active{};
        bool* target_bool{};
        int* target_int{};
        float* target_float{};
        std::vector<int>* target_indices{};
        const std::vector<std::string>* items{};
        float v_min{ 0.f };
        float v_max{ 0.f };
        float scale{ 1.f };
        std::string format;
    };

    std::vector<Entry> GetEntries();
    std::string EntryCategory(const std::string& group);
    void SetEnabled(uint32_t uid, bool on);
    void Remove(uint32_t uid);
    void ResetAll();

    void BeginCapture(int* key);
    void CancelCapture();
    bool IsCapturing(int* key);
    bool IsCapturingBind();
    void PollCapture();
    const char* KeyName(int key);

    void Update();

    void RenderCheckboxPopup(const char* popup_name, std::string_view label, bool* value);
    void RenderCheckboxPopup(const char* popup_name, std::string_view label, int* value);
    void RenderSliderIntPopup(const char* popup_name, std::string_view label, int* value, int v_min, int v_max, const char* format);
    void RenderSliderFloatPopup(const char* popup_name, std::string_view label, float* value, float v_min, float v_max, const char* format, float scale = 1.f);
    void RenderDropdownPopup(const char* popup_name, std::string_view label, int* value, const std::vector<std::string>& items);
    void RenderMultiDropdownPopup(const char* popup_name, std::string_view label, std::vector<bool>* value, const std::vector<std::string>& items);
    void RenderMultiDropdownPopup(const char* popup_name, std::string_view label, std::vector<int>* value, const std::vector<std::string>& items);

    struct ActiveBind {
        std::string name;
        std::string key_name;
        bool active = false;
    };

    std::vector<ActiveBind> GetActiveList();

    void SaveToConfig();
    void LoadFromConfig();
}

namespace ConfigPopup {
    inline ImVec2 menu_pos{ 0.f, 0.f };
    inline ImVec2 menu_size{ 0.f, 0.f };
    inline bool open{ false };

    inline void SetMenuRect(const ImVec2& pos, const ImVec2& size) {
        menu_pos = pos;
        menu_size = size;
    }
}

struct CGui {
    template <typename T>
    T* AnimContainer(T** state_ptr, ImGuiID id) {
        T* state = static_cast<T*>(GetStateStorage()->GetVoidPtr(id));
        if (!state) {
            auto owned_state = std::make_shared<T>();
            state = owned_state.get();
            this->animation_states.emplace_back(std::move(owned_state));
            GetStateStorage()->SetVoidPtr(id, state);
        }

        *state_ptr = state;
        return state;
    }

    struct ListboxState {
        float alpha;
        bool opened;
    };

    float menu_anim;
    int esp_target = 0;

    bool LabelPopupItem(const char* label, const char* popup_id);

    float FixedSpeed(float speed) { return speed / ImGui::GetIO().Framerate; };

    bool Begin(std::string_view name, bool* p_open = nullptr, ImGuiWindowFlags flags = ImGuiWindowFlags_None);

    void End();

    void PushStyleColor(ImGuiCol idx, ImU32 col);

    void PopStyleColor(int count = 1);

    void PushStyleVar(ImGuiStyleVar idx, float val);

    void PushStyleVar(ImGuiStyleVar idx, const ImVec2& val);

    void PopStyleVar(int count = 1);

    void PushFont(ImFont* font);

    void PopFont();

    void SetCursorPos(const ImVec2& local_pos);

    void BeginGroup();

    void EndGroup();

    void BeginContent();

    void EndContent();

    void Sameline();

    bool BeginDefChild(std::string_view name, const ImVec2& size_arg = ImVec2(0, 0), ImGuiChildFlags child_flags = 0, ImGuiWindowFlags window_flags = 0);

    void EndDefChild();

    void SetNextWindowPos(const ImVec2& pos, ImGuiCond cond = 0, const ImVec2& pivot = ImVec2(0, 0));

    void SetNextWindowSize(const ImVec2& size, ImGuiCond cond = 0);

    void BeginChild(std::string_view name, const char* fa_icon = nullptr, const ImVec2& size = ImVec2(0, 0));

    void EndChild();

    bool Section(std::string_view label, std::string_view icon, int section_id, int& section_variable, bool last);

    bool SubSection(std::string_view label, std::string_view icon, int section_id, int& section_variable);

    bool Checkbox(std::string_view label, bool* callback);

    bool CheckboxEx(std::string_view label, bool* callback, bool insecure = false, Color_t** colors = nullptr, int color_count = 0, std::function<void()> gear_fn = nullptr);

    bool CheckboxHitmarkerEx(std::string_view label, bool* callback, bool insecure = false, Color_t** colors = nullptr, int color_count = 0, bool is_world_hitmarker = false, std::function<void()> gear_fn = nullptr);

    bool CheckboxWithDropdown(std::string_view label, bool* callback, int* current_item, const std::vector<std::string>& items, bool insecure = false, std::function<void()> gear_fn = nullptr);

    bool CheckboxHitmarkers(std::string_view label, bool* callback, Color_t* color, bool* glow, Color_t* glow_color, float* glow_intensity, float* lifetime, float* thickness, float* size, float* gap);

    bool SliderFloat(std::string_view label, float* v, float v_min, float v_max, const char* format = "%.1f", float scale = 1.f);

    bool SliderInt(std::string_view label, int* v, int v_min, int v_max, const char* format = "%d");

    bool SliderFloatEx(std::string_view label, float* v, float v_min, float v_max, std::function<void()> gear_fn = nullptr, const char* format = "%.1f", float scale = 1.f);

    bool SliderIntEx(std::string_view label, int* v, int v_min, int v_max, std::function<void()> gear_fn = nullptr, const char* format = "%d");

    bool Dropdown(std::string_view label, int* current_item, const std::vector<std::string>& items);

    bool DropdownNew(const char* label, int* current_item, const char* const items[], int items_count);

    bool DropdownEx(std::string_view label, int* current_item, const std::vector<std::string>& items, std::function<void()> gear_fn = nullptr);

    void DropdownWeaponConfig(const char* label);

    bool CheckboxHitsounds(const char* label, bool* enable_var, std::function<void()> gear_fn);

    void MultiDropdown(std::string_view label, std::vector<bool>* variable, const std::vector<std::string>& items, std::function<void()> gear_fn = nullptr);

    void MultiDropdown(std::string_view label, std::vector<int>* variable, const std::vector<std::string>& items);

    void MultiCombo(std::string_view label, std::vector<bool>* variable, const std::vector<std::string>& items);

    bool SelectableEx(std::string_view label, bool active, const ImVec2& size);

    bool Selectable(std::string_view label, bool* p_selected, const ImVec2& size);

    bool BeginListBox(const char* label, const ImVec2& size_arg);

    bool BeginListBox(const char* label, int items_count = 0, int height_in_items = 0);

    void EndListBox();

    bool TextField(std::string_view label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0);

    bool Buttton(std::string_view label, const ImVec2& size_arg = ImVec2(0, 0));

    void Update();

    bool ColorEdit(std::string_view label, Color_t** colors, int color_count, bool alpha = true);

    bool Keybind(std::string_view label, int* key, int* mode);

    void ElementTooltip(const char* text, float delay = 0.5f);

    bool ListEx(std::string_view label, std::function<void()> gear_fn);

    bool ListEx(std::string_view label, Color_t** colors, int color_count, std::function<void()> gear_fn);

    void RenderConfigModal(bool* open);

    void Render();

    void RenderEspPreview(float alpha, const ImVec2& size_arg, const std::string& prefix);

    void RenderRagebotContent();

    void RenderInitOverlay();

    void RenderConfigPopup();

    void RenderAboutPopup(ImVec2 menu_pos, ImVec2 menu_size, bool& open, float anim);

    bool DropdownWeapons(std::string_view label, int* current_item, float width);

    bool SliderTrack(const char* str_id, const ImVec2& bb_min, const ImVec2& bb_max, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, float alpha = 1.f);

    bool VectorScalar(std::string_view label, Vector* v, float min_x, float max_x, float min_y, float max_y, float min_z, float max_z);

    // tabs

    void RenderLegitbotContent();

    void RenderVisualsContent();

    void RenderWorldContent();

    void RenderWorldAmbienceContent();

    void RenderWorldPerspectiveContent();

    void RenderMiscContent();

    void RenderSkinsContent();

    void RenderConfigurationsContent();

    void RenderScriptsContent();

    void RenderHotkeysContent();

    std::vector<std::string> g_child_stack;
    std::string s_deferred_tooltip{ "" };
    ImVec2 menu_pos = ImVec2(0.f, 0.f);
    ImVec2 menu_size = ImVec2(0.f, 0.f);
    float menu_rounding = 0.f;
    float ui_alpha = 0.f;
    float about_ease = 0.f;
    bool about_active = false;
    ImDrawList* about_draw_list = nullptr;
    std::vector<ImDrawList*> about_extra_draw_lists;
    std::vector<void*> about_extra_windows;
    std::vector<std::shared_ptr<void>> animation_states;
};

inline CGui* Gui = new CGui();

enum fade_direction : int {
    vertically,
    horizontally,
    diagonally,
    diagonally_reversed,
};

enum class EKeyState : int {
    none,
    down,
    up,
    released
};

struct Vector;
struct Vector2D;

struct CDraw {
    bool opened = false;
    bool initialized = false;
    HWND window = nullptr;
    std::vector<ID3D11ShaderResourceView*> frame_texture_refs;

    ImU32 GetClr(const ImVec4& col, float alpha = 1.f);

    void Text(ImDrawList* draw_list, const ImFont* font, float font_size, const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end = NULL, float wrap_width = NULL, const ImVec4* cpu_fine_clip_rect = 0);

    void TextClipped(ImDrawList* draw_list, ImFont* font, const ImVec2& pos_min, const ImVec2& pos_max, ImU32 color, const char* text, const char* text_display_end = NULL, const ImVec2* text_size_if_known = NULL, const ImVec2& align = ImVec2(0.f, 0.f), const ImRect* clip_rect = NULL);

    void RadialGradient(ImDrawList* draw_list, const ImVec2& center, float radius, ImU32 col_in, ImU32 col_out);

    void SetLinearColorAlpha(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, ImVec2 gradient_p0, ImVec2 gradient_p1, ImU32 col0, ImU32 col1);

    void Line(ImDrawList* draw_list, const ImVec2& p1, const ImVec2& p2, ImU32 col, float thickness = 1.0f);

    //void RadialGradient2d(ImDrawList* draw_list, Vector center, float radius, Color in, Color out);

    void Rect(ImDrawList* draw_list, const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding = 0.0f, ImDrawFlags flags = 0, float thickness = 1.0f);

    void RectFilled(ImDrawList* draw_list, const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding = 0.0f, ImDrawFlags flags = 0);

    void RectFilledMultiColor(ImDrawList* draw, const ImVec2& p_min, const ImVec2& p_max, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left, float rounding = 0.f, ImDrawFlags flags = 0);

    void FadeRectFilled(ImDrawList* draw, const ImVec2& pos_min, const ImVec2& pos_max, ImU32 col_one, ImU32 col_two, fade_direction direction, float rounding = 0.f, ImDrawFlags flags = 0);

    void ShadowRect(ImDrawList* draw_list, const ImVec2& obj_min, const ImVec2& obj_max, ImU32 shadow_col, float shadow_thickness, const ImVec2& shadow_offset, ImDrawFlags flags = 0, float obj_rounding = 0.0f);

    void Circle(ImDrawList* draw_list, const ImVec2& center, float radius, ImU32 col, int num_segments = 0, float thickness = 1.0f);

    void CircleFilled(ImDrawList* draw_list, const ImVec2& center, float radius, ImU32 col, int num_segments = 0);

    void ShadowCircle(ImDrawList* draw_list, const ImVec2& obj_center, float obj_radius, ImU32 shadow_col, float shadow_thickness, const ImVec2& shadow_offset, ImDrawFlags flags = 0, int obj_num_segments = 12);

    void Triangle(ImDrawList* draw_list, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col, float thickness = 1.f);

    void TriangleFilled(ImDrawList* draw_list, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col);

    void Image(ImDrawList* draw_list, ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min = ImVec2(0, 0), const ImVec2& uv_max = ImVec2(1, 1), ImU32 col = IM_COL32_WHITE);

    void ImageRounded(ImDrawList* draw_list, ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min = ImVec2(0, 0), const ImVec2& uv_max = ImVec2(1, 1), ImU32 col = IM_COL32_WHITE, float rounding = 1.f, ImDrawFlags flags = 0);

    void ShadowConvexPoly(ImDrawList* draw_list, const ImVec2* points, int points_count, ImU32 shadow_col, float shadow_thickness, const ImVec2& shadow_offset, ImDrawFlags flags = 0);

    void ShadowNgon(ImDrawList* draw_list, const ImVec2& obj_center, float obj_radius, ImU32 shadow_col, float shadow_thickness, const ImVec2& shadow_offset, ImDrawFlags flags, int obj_num_segments);

    void RotateStart();

    void RotateEnd(float rad, ImVec2 center = ImVec2(0, 0));

    float DegToRad(float deg);

    void PushClipRect(ImDrawList* draw_list, const ImVec2& clip_rect_min, const ImVec2& clip_rect_max, bool intersect_with_current_clip_rect = false);

    void PopClipRect(ImDrawList* draw_list);

    void BeginFrame();

    void EndFrame();

    void SubmitDrawData(ImDrawData* draw_data);

    bool QueueDirectText(const ImVec2& pos, ImU32 color, const char* text, const ImFont* font, float font_size);

    void SubmitDirectText(IDXGISwapChain* swap_chain);

    void InvalidateDirectTextTarget();

    void ShutdownDirectText();

    void KeepTextureAlive(ID3D11ShaderResourceView* texture_view);

    void ReleaseFrameTextures();

    void InitFonts();

    void Capture();

    void Clear();

    Vector2D WorldToScreen(const Vector& pos);
};

inline CDraw* Draw = new CDraw();

struct CInput {
    WNDPROC old_wnd_proc = nullptr;
    std::array<EKeyState, 256U + 5U> arr_key_state = {};

    bool Init();
    void Restore();
    bool IsKeyReleased(const uint32_t button_code);
    bool OnWndProc(UINT msg, WPARAM wparam, LPARAM lparam);
};

inline CInput* Input = new CInput();
