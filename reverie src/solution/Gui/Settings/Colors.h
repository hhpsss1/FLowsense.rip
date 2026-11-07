#pragma once

#undef max
#undef min

#include <cstdint>
#include <array>
#include <algorithm>
#include <cmath>
#include <tuple>
#include <imgui/imgui.h>

struct Color_t {
    constexpr Color_t() : r(0), g(0), b(0), a(255) {}

    constexpr Color_t(int r, int g, int b, int a = 255) :
        r(static_cast<uint8_t>(std::clamp(r, 0, 255))),
        g(static_cast<uint8_t>(std::clamp(g, 0, 255))),
        b(static_cast<uint8_t>(std::clamp(b, 0, 255))),
        a(static_cast<uint8_t>(std::clamp(a, 0, 255))) {
    }

    constexpr Color_t(float r, float g, float b, float a = 1.0f) :
        r(static_cast<uint8_t>(std::clamp(r * 255.0f, 0.0f, 255.0f))),
        g(static_cast<uint8_t>(std::clamp(g * 255.0f, 0.0f, 255.0f))),
        b(static_cast<uint8_t>(std::clamp(b * 255.0f, 0.0f, 255.0f))),
        a(static_cast<uint8_t>(std::clamp(a * 255.0f, 0.0f, 255.0f))) {
    }

    [[nodiscard]] constexpr uint8_t R() const { return r; }
    [[nodiscard]] constexpr uint8_t G() const { return g; }
    [[nodiscard]] constexpr uint8_t B() const { return b; }
    [[nodiscard]] constexpr uint8_t A() const { return a; }

    [[nodiscard]] constexpr float Rf() const { return r / 255.0f; }
    [[nodiscard]] constexpr float Gf() const { return g / 255.0f; }
    [[nodiscard]] constexpr float Bf() const { return b / 255.0f; }
    [[nodiscard]] constexpr float Af() const { return a / 255.0f; }

    constexpr Color_t& SetR(uint8_t value) { r = value; return *this; }
    constexpr Color_t& SetG(uint8_t value) { g = value; return *this; }
    constexpr Color_t& SetB(uint8_t value) { b = value; return *this; }
    constexpr Color_t& SetA(uint8_t value) { a = value; return *this; }

    constexpr Color_t& SetRf(float value) { r = static_cast<uint8_t>(std::clamp(value * 255.0f, 0.0f, 255.0f)); return *this; }
    constexpr Color_t& SetGf(float value) { g = static_cast<uint8_t>(std::clamp(value * 255.0f, 0.0f, 255.0f)); return *this; }
    constexpr Color_t& SetBf(float value) { b = static_cast<uint8_t>(std::clamp(value * 255.0f, 0.0f, 255.0f)); return *this; }
    constexpr Color_t& SetAf(float value) { a = static_cast<uint8_t>(std::clamp(value * 255.0f, 0.0f, 255.0f)); return *this; }

    constexpr Color_t(const Color_t& color_second, float a) :
        r(color_second.r),
        g(color_second.g),
        b(color_second.b),
        a(static_cast<uint8_t>(std::clamp(a * 255.0f, 0.0f, 255.0f))) {
    }

    [[nodiscard]] uint32_t ToU32(float alpha_multiplier = 1.0f) const;
    [[nodiscard]] ImVec4 ToVec4(float alpha_multiplier = 1.0f) const;
    [[nodiscard]] constexpr uint32_t ToHex() const { return (static_cast<uint32_t>(r) << 24) | (static_cast<uint32_t>(g) << 16) | (static_cast<uint32_t>(b) << 8) | static_cast<uint32_t>(a); }
    [[nodiscard]] std::array<float, 4> ToArray() const { return { Rf(), Gf(), Bf(), Af() }; }

    static Color_t FromHsv(float hue, float saturation, float value) {
        const float h = std::fmod(hue, 1.0f) * 6.0f;
        const int i = static_cast<int>(std::floor(h));
        const float f = h - i;
        const float p = value * (1.0f - saturation);
        const float q = value * (1.0f - saturation * f);
        const float C_T = value * (1.0f - saturation * (1.0f - f));

        float r, g, b;
        switch (i % 6) {
        case 0: r = value; g = C_T; b = p; break;
        case 1: r = q; g = value; b = p; break;
        case 2: r = p; g = value; b = C_T; break;
        case 3: r = p; g = q; b = value; break;
        case 4: r = C_T; g = p; b = value; break;
        default: r = value; g = p; b = q; break;
        }
        return Color_t(r, g, b);
    }

    [[nodiscard]] std::tuple<float, float, float> ToHsv() const {
        const float r = Rf(), g = Gf(), b = Bf();
        const float max_val = std::max({ r, g, b });
        const float min_val = std::min({ r, g, b });
        const float delta = max_val - min_val;

        float h = 0.0f;
        if (delta > 0.0f) {
            if (max_val == r)
                h = std::fmod((g - b) / delta, 6.0f);
            else if (max_val == g)
                h = (b - r) / delta + 2.0f;
            else
                h = (r - g) / delta + 4.0f;
            h /= 6.0f;
        }

        const float s = (max_val > 0.0f) ? delta / max_val : 0.0f;
        return { h, s, max_val };
    }

    constexpr bool operator==(const Color_t& other) const { return r == other.r && g == other.g && b == other.b && a == other.a; }
    constexpr bool operator!=(const Color_t& other) const { return !(*this == other); }
    constexpr Color_t operator+(const Color_t& other) const { return Color_t(std::min(255, static_cast<int>(r) + static_cast<int>(other.r)), std::min(255, static_cast<int>(g) + static_cast<int>(other.g)), std::min(255, static_cast<int>(b) + static_cast<int>(other.b)), std::min(255, static_cast<int>(a) + static_cast<int>(other.a))); }

    constexpr Color_t& operator+=(const Color_t& other) {
        r = static_cast<uint8_t>(std::min(255, static_cast<int>(r) + static_cast<int>(other.r)));
        g = static_cast<uint8_t>(std::min(255, static_cast<int>(g) + static_cast<int>(other.g)));
        b = static_cast<uint8_t>(std::min(255, static_cast<int>(b) + static_cast<int>(other.b)));
        a = static_cast<uint8_t>(std::min(255, static_cast<int>(a) + static_cast<int>(other.a)));
        return *this;
    }

    constexpr Color_t operator*(float multiplier) const { return Color_t(static_cast<uint8_t>(std::min(255.0f, static_cast<float>(r) * multiplier)), static_cast<uint8_t>(std::min(255.0f, static_cast<float>(g) * multiplier)), static_cast<uint8_t>(std::min(255.0f, static_cast<float>(b) * multiplier)), static_cast<uint8_t>(std::min(255.0f, static_cast<float>(a) * multiplier))); }

    constexpr Color_t& operator*=(float multiplier) {
        r = static_cast<uint8_t>(std::min(255.0f, static_cast<float>(r) * multiplier));
        g = static_cast<uint8_t>(std::min(255.0f, static_cast<float>(g) * multiplier));
        b = static_cast<uint8_t>(std::min(255.0f, static_cast<float>(b) * multiplier));
        a = static_cast<uint8_t>(std::min(255.0f, static_cast<float>(a) * multiplier));
        return *this;
    }

    constexpr uint8_t& operator[](size_t index) { return index == 0 ? r : (index == 1 ? g : (index == 2 ? b : a)); }

    constexpr const uint8_t& operator[](size_t index) const { return index == 0 ? r : (index == 1 ? g : (index == 2 ? b : a)); }

    ImU32 GetU32(float alpha_multiplier = -1.f) const {
        uint8_t alpha = static_cast<uint8_t>(std::clamp(this->a * alpha_multiplier, 0.0f, 255.0f));
        return IM_COL32(this->r, this->g, this->b, alpha);
    }

    ImVec4 GetVec4(float alpha_multiplier = -1.f) const {
        float alpha = std::clamp(this->Af() * alpha_multiplier, 0.0f, 1.0f);
        return ImVec4(this->Rf(), this->Gf(), this->Bf(), alpha);
    }

    static constexpr Color_t White() { return Color_t(255, 255, 255, 255); }
    static constexpr Color_t Black() { return Color_t(0, 0, 0, 255); }
    static constexpr Color_t Red() { return Color_t(255, 0, 0, 255); }
    static constexpr Color_t Green() { return Color_t(0, 255, 0, 255); }
    static constexpr Color_t Blue() { return Color_t(0, 0, 255, 255); }
    static constexpr Color_t Yellow() { return Color_t(255, 255, 0, 255); }
    static constexpr Color_t Cyan() { return Color_t(0, 255, 255, 255); }
    static constexpr Color_t Magenta() { return Color_t(255, 0, 255, 255); }
    static constexpr Color_t Transparent() { return Color_t(0, 0, 0, 0); }

    uint8_t r, g, b, a;
};

struct Color4_f {
    float r, g, b, a;
};

struct Colors_t {
    ImColor accent{ 110, 200, 110 };

    int current_theme{ 0 };

    ImColor text{ 255, 255, 255 };
    ImColor text_secondary{ 150, 150, 155 };
    ImColor text_faint{ 110, 110, 115 };
    ImColor divider{ 255, 255, 255 };

    struct {
        ImColor background{ 13, 13, 13 };
        ImColor bar{ 25, 25, 25 };
        ImColor name{ 255, 255, 255 };
        ImColor section_name{ 255, 255, 255 };
    } window;

    struct {
        ImColor background{ 19, 19, 19 };
        ImColor bar{ 19, 19, 19 };
        ImColor name{ 255, 255, 255 };
    } child;

    struct {
        ImColor background{ 25, 25, 25 };
        ImColor text_inactive{ 100, 100, 100 };
        ImColor text{ 255, 255, 255 };
    } section;

    struct {
        ImColor text_inactive{ 100, 100, 100 };
        ImColor text{ 255, 255, 255 };
    } sub_section;

    struct {
        ImColor background{ 33, 33, 33 };
        ImColor circle_active{ 255, 255, 255 };
        ImColor circle_inactive{ 100, 100, 100 };
        ImColor text{ 255, 255, 255 };
    } checkbox;

    struct {
        ImColor background{ 33, 33, 33 };
        ImColor text{ 255, 255, 255 };
        ImColor value{ 100, 100, 100 };
        ImColor circle{ 255, 255, 255 };
    } slider;

    struct {
        ImColor background{ 33, 33, 33 };
        ImColor text{ 255, 255, 255 };
        ImColor value{ 100, 100, 100 };
    } dropdown;

    struct {
        ImColor text{ 255, 255, 255 };
        ImColor text_inactive{ 100, 100, 100 };
    } selectable;

    struct {
        ImColor background{ 33, 33, 33 };
        ImColor value{ 255, 255, 255 };
        ImColor text{ 100, 100, 100 };
    } text_field;

    struct {
        ImColor background{ 33, 33, 33 };
        ImColor text{ 255, 255, 255 };
    } button;

    struct {
        ImColor background{ 33, 33, 33 };
        ImColor text{ 255, 255, 255 };
    } color_edit;

    struct {
        ImColor background{ 33, 33, 33 };
        ImColor text{ 255, 255, 255 };
        ImColor value{ 100, 100, 100 };
    } keybind;

    struct {
        ImColor background{ 25, 25, 25 };
        ImColor text{ 255, 255, 255 };
        ImColor text_inactive{ 100, 100, 100 };
        ImColor hovered{ 35, 35, 35 };
        ImColor active{ 123, 137, 255 };
        ImColor border{ 35, 35, 35 };
    } listbox;
};

inline Colors_t* Colors = new Colors_t();