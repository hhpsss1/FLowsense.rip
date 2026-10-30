#include <shared.h>

inline std::string SafeStr(const char* s) { return s ? std::string(s) : std::string(); }

namespace {
    using namespace esp_layout;

    static std::unordered_map<uintptr_t, esp_flags::State_t> flags_state;

    struct AmmoAnim_t {
        float visual_ammo = -1.f;
        float reload_timer = 0.f;
        bool was_reloading = false;
    };

    static std::unordered_map<uintptr_t, AmmoAnim_t> ammo_anims;

    inline int DefIndexToGrenadeIconIndex(uint16_t def_idx) {
        switch (def_idx) {
        case 44: return 0;
        case 43: return 1;
        case 46:
        case 48: return 2;
        case 47: return 3;
        case 45: return 4;
        default: return -2;
        }
    }

    struct EspGrenadeInfo_t {
        int icon_index;
        bool is_active;
        CMisc::Panorama_t::IconData_t icon_data;
    };

    std::vector<EspGrenadeInfo_t> GatherPlayerGrenades(C_CSPlayerPawn* pawn, C_CSWeaponBase* active_weapon) {
        std::vector<EspGrenadeInfo_t> list;
        if (!pawn)
            return list;

        auto* ws = pawn->m_pWeaponServices();
        if (!ws)
            return list;

        uint16_t active_def_idx = 0;
        if (active_weapon && active_weapon->m_AttributeManager() && active_weapon->m_AttributeManager()->m_Item())
            active_def_idx = active_weapon->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex();

        for (int i = 0; i < ws->m_hMyWeapons().size; i++) {
            if (!ws->m_hMyWeapons().elements[i].Valid())
                continue;

            C_CSWeaponBase* w = ws->m_hMyWeapons().elements[i].get<C_CSWeaponBase>();
            if (!w || !w->m_AttributeManager() || !w->m_AttributeManager()->m_Item())
                continue;

            uint16_t idx = w->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex();
            int icon_idx = DefIndexToGrenadeIconIndex(idx);
            if (icon_idx < -1)
                continue;

            bool active = (w == active_weapon) || (idx == active_def_idx);

            CMisc::Panorama_t::IconData_t icon = Misc->Panorama.GetGrenadeIcon(icon_idx);
            if (icon.texture_view)
                list.push_back({ icon_idx, active, icon });
        }
        return list;
    }

    Dims_t MeasureGrenadeIconsDims(const std::vector<EspGrenadeInfo_t>& grenades, float icon_h = 10.f, float gap = 3.f) {
        if (grenades.empty())
            return { 0.f, 0.f, false };

        float total_w = 0.f;
        for (size_t i = 0; i < grenades.size(); i++) {
            float w = icon_h * 1.2f;
            if (grenades[i].icon_data.width > 0 && grenades[i].icon_data.height > 0)
                w = icon_h * ((float)grenades[i].icon_data.width / (float)grenades[i].icon_data.height);

            total_w += w;
            if (i > 0)
                total_w += gap;
        }
        return { total_w, icon_h, false };
    }

    ImVec2 RoundPos(const ImVec2& p) {
        return ImVec2(std::floorf(p.x + 0.5f), std::floorf(p.y + 0.5f));
    }

    void AddTextFullOutline(ImDrawList* draw, ImFont* font, float font_size, const ImVec2& pos, ImU32 col, const char* text, ImU32 outline_col = IM_COL32(0, 0, 0, 255)) {
        ImVec2 p = ImVec2(floorf(pos.x), floorf(pos.y));

        draw->AddText(font, font_size, ImVec2(p.x - 1.f, p.y), outline_col, text);
        draw->AddText(font, font_size, ImVec2(p.x + 1.f, p.y), outline_col, text);
        draw->AddText(font, font_size, ImVec2(p.x, p.y - 1.f), outline_col, text);
        draw->AddText(font, font_size, ImVec2(p.x, p.y + 1.f), outline_col, text);

        draw->AddText(font, font_size, ImVec2(p.x - 1.f, p.y - 1.f), outline_col, text);
        draw->AddText(font, font_size, ImVec2(p.x + 1.f, p.y - 1.f), outline_col, text);
        draw->AddText(font, font_size, ImVec2(p.x - 1.f, p.y + 1.f), outline_col, text);
        draw->AddText(font, font_size, ImVec2(p.x + 1.f, p.y + 1.f), outline_col, text);

        draw->AddText(font, font_size, p, col, text);
    }

    void AddTextOutlined(ImDrawList* draw_list, ImFont* font, float font_size, const ImVec2& pos, ImU32 text_col, const char* text) {
        ImVec2 p = RoundPos(pos);
        int text_a = (text_col >> IM_COL32_A_SHIFT) & 0xFF;

        if (font == Var->font.pixel)
            return AddTextFullOutline(draw_list, font, font_size, pos, text_col, text, IM_COL32(0, 0, 0, (int)(180 * (text_a / 255.f))));

        draw_list->AddText(font, font_size, ImVec2(p.x + 2.f, p.y + 2.f), IM_COL32(0, 0, 0, (int)(40 * (text_a / 255.f))), text);
        draw_list->AddText(font, font_size, ImVec2(p.x + 1.f, p.y + 1.f), IM_COL32(0, 0, 0, (int)(180 * (text_a / 255.f))), text);
        draw_list->AddText(font, font_size, p, text_col, text);
    }

    float GlowCurve(float t) {
        t = std::clamp(t, 0.f, 1.f);
        float f = 1.f - t;
        return f * f;
    }

    void AddBoxGlow(ImDrawList* draw_list, const ImVec2& min, const ImVec2& max, ImU32 col, float size = 6.f) {
        if (!draw_list || (col & IM_COL32_A_MASK) == 0 || max.x <= min.x || max.y <= min.y)
            return;

        int r = (col >> IM_COL32_R_SHIFT) & 0xFF;
        int g = (col >> IM_COL32_G_SHIFT) & 0xFF;
        int b = (col >> IM_COL32_B_SHIFT) & 0xFF;
        int base_a = (col >> IM_COL32_A_SHIFT) & 0xFF;

        auto make = [&](float t) -> ImU32 {
            int a = (int)(base_a * 0.35f * GlowCurve(t));
            return IM_COL32(r, g, b, a);
        };

        const int steps = 4;
        const float step = size / (float)steps;

        for (int i = 0; i < steps; i++) {
            float o0 = step * i;
            float o1 = step * (i + 1);
            float t0 = (float)i / (float)steps;
            float t1 = (float)(i + 1) / (float)steps;
            ImU32 c0 = make(t0);
            ImU32 c1 = make(t1);

            draw_list->AddRectFilledMultiColor(ImVec2(min.x - o0, min.y - o1), ImVec2(max.x + o0, min.y - o0), c1, c1, c0, c0);
            draw_list->AddRectFilledMultiColor(ImVec2(min.x - o0, max.y + o0), ImVec2(max.x + o0, max.y + o1), c0, c0, c1, c1);
            draw_list->AddRectFilledMultiColor(ImVec2(min.x - o1, min.y - o0), ImVec2(min.x - o0, max.y + o0), c1, c0, c0, c1);
            draw_list->AddRectFilledMultiColor(ImVec2(max.x + o0, min.y - o0), ImVec2(max.x + o1, max.y + o0), c0, c1, c1, c0);
        }
    }

    ImFont* PickFont(int idx) {
        if (idx == 0 && Var->font.verdana)
            return Var->font.verdana;

        if (idx == 1 && Var->font.verdanab)
            return Var->font.verdanab;

        if (idx == 2 && Var->font.pixel)
            return Var->font.pixel;

        return Var->font.verdana ? Var->font.verdana : ImGui::GetFont();
    }

    Dims_t MeasureText(ImFont* font, const std::string& s) {
        if (!font || s.empty())
            return { 0.f, 0.f, false };

        ImVec2 sz = font->CalcTextSizeA(font->LegacySize, std::numeric_limits<float>::max(), 0.f, s.c_str());
        return { sz.x, sz.y, false };
    }

    Dims_t MeasureIcon(const CMisc::Panorama_t::IconData_t& icon) {
        const float h = 10.f;
        float w = h * 2.4f;

        if (icon.texture_view && icon.width > 0 && icon.height > 0)
            w = h * ((float)icon.width / (float)icon.height);

        return { w, h, false };
    }

    uint16_t WeaponDefIndexOf(C_CSWeaponBase* w) {
        if (!w || !w->m_AttributeManager() || !w->m_AttributeManager()->m_Item())
            return 0;

        return w->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex();
    }

    esp_flags::Input_t GatherFlagsInput(CCSPlayerController* controller, C_CSPlayerPawn* pawn, C_CSWeaponBase* active_weapon, bool active_is_grenade) {
        esp_flags::Input_t in{};

        in.scoped = pawn->m_bIsScoped();

        if (auto* ws = pawn->m_pWeaponServices()) {
            for (int i = 0; i < ws->m_hMyWeapons().size; i++) {
                if (!ws->m_hMyWeapons().elements[i].Valid())
                    continue;

                C_CSWeaponBase* w = ws->m_hMyWeapons().elements[i].get<C_CSWeaponBase>();
                if (!w)
                    continue;

                uint16_t idx = WeaponDefIndexOf(w);

                if (idx == 49)
                    in.bomb = true;

                if (idx == 31)
                    in.taser = true;
            }
        }

        in.reload = active_weapon && active_weapon->m_iClip1() == 0 && active_weapon->m_pReserveAmmo() > 0;

        if (auto* item_svc = pawn->m_pItemServices())
            in.defuser = item_svc->m_bHasDefuser();

        auto grenade = reinterpret_cast<C_BaseCSGrenade*>(active_weapon);
        in.pin = active_is_grenade && grenade && grenade->m_bPinPulled();

        if (auto* money_svc = controller->m_pInGameMoneyServices())
            in.money = money_svc->m_iAccount();

        in.ping = controller->m_iPing();
        in.armor = pawn->m_ArmorValue();
        in.helmet = pawn->m_pItemServices() && pawn->m_pItemServices()->m_bHasHelmet();
        in.velocity_modifier = pawn->m_flVelocityModifier();

        return in;
    }

    void DrawBar(ImDrawList* dl, const Rect_t& r, int side, float cur_val, float max_val, float alpha, bool glow_on, bool grad_on, Color_t bar_col, Color_t grad_col, ImFont* font, int text_mode, bool is_ammo = false, bool is_reloading = false) {
        if (max_val <= 0.f)
            return;

        float pct = std::clamp(cur_val / max_val, 0.f, 1.f);
        bool vertical = (side == side_left || side == side_right);

        ImVec2 bmin = RoundPos(r.min);
        ImVec2 bmax = RoundPos(r.max);

        ImU32 bg = IM_COL32(0, 0, 0, (int)(110 * alpha));
        ImU32 outline = IM_COL32(0, 0, 0, (int)(160 * alpha));

        dl->AddRectFilled(ImVec2(bmin.x - 1.f, bmin.y - 1.f), ImVec2(bmax.x + 1.f, bmax.y + 1.f), outline);
        dl->AddRectFilled(bmin, bmax, bg);

        ImVec2 fmin, fmax;

        if (vertical) {
            float bh = bmax.y - bmin.y;
            float fh = bh * pct;
            fmin = ImVec2(bmin.x, bmax.y - fh);
            fmax = ImVec2(bmax.x, bmax.y);
        } else {
            float bw = bmax.x - bmin.x;
            float fw = bw * pct;
            fmin = ImVec2(bmin.x, bmin.y);
            fmax = ImVec2(bmin.x + fw, bmax.y);
        }

        if (is_ammo && is_reloading) {
            glow_on = true;
            float pulse = 0.75f + 0.25f * sinf((float)ImGui::GetTime() * 8.0f);
            alpha *= pulse;
        }

        ImU32 col = IM_COL32((int)(bar_col.Rf() * 255.f), (int)(bar_col.Gf() * 255.f), (int)(bar_col.Bf() * 255.f), (int)(bar_col.Af() * 255.f * alpha));
        ImU32 col_grad = IM_COL32((int)(grad_col.Rf() * 255.f), (int)(grad_col.Gf() * 255.f), (int)(grad_col.Bf() * 255.f), (int)(grad_col.Af() * 255.f * alpha));
        bool has_fill = vertical ? (fmax.y > fmin.y) : (fmax.x > fmin.x);

        if (glow_on && has_fill)
            AddBoxGlow(dl, fmin, fmax, col, 4.f);

        if (has_fill) {
            if (grad_on) {
                if (vertical)
                    dl->AddRectFilledMultiColor(fmin, fmax, col_grad, col_grad, col, col);
                else
                    dl->AddRectFilledMultiColor(fmin, fmax, col, col_grad, col_grad, col);
            } else {
                dl->AddRectFilled(fmin, fmax, col);
            }

            if (is_ammo && is_reloading) {
                float shimmer_t = fmodf((float)ImGui::GetTime() * 2.2f, 1.0f);
                ImU32 shimmer_col = IM_COL32(255, 255, 255, (int)(210 * alpha));

                if (vertical) {
                    float y_pos = fmax.y - (fmax.y - fmin.y) * shimmer_t;
                    dl->AddLine(ImVec2(fmin.x, y_pos), ImVec2(fmax.x, y_pos), shimmer_col, 1.5f);
                } else {
                    float x_pos = fmin.x + (fmax.x - fmin.x) * shimmer_t;
                    dl->AddLine(ImVec2(x_pos, fmin.y), ImVec2(x_pos, fmax.y), shimmer_col, 1.5f);
                }
            }
        }

        bool should_draw_text = false;
        if (is_ammo) {
            if (text_mode == 1)
                should_draw_text = true;
        } else {
            if (text_mode == 1 && cur_val < max_val && cur_val > 0.f)
                should_draw_text = true;
            else if (text_mode == 2)
                should_draw_text = true;
        }

        if (font && should_draw_text) {
            std::string val_str;

            if (is_ammo)
                val_str = std::to_string((int)std::round(pct * 100.f));
            else
                val_str = std::to_string((int)std::round(cur_val));

            if (!val_str.empty()) {
                Dims_t measured = MeasureText(font, val_str);
                ImVec2 tsz(measured.width, measured.height);
                ImVec2 tp;

                if (side == side_left) {
                    float maxb = std::max(bmin.y, bmax.y - tsz.y);
                    tp = ImVec2(bmin.x - tsz.x * 0.5f, std::clamp(fmin.y - tsz.y * 0.5f, bmin.y, maxb));
                } else if (side == side_right) {
                    float maxb = std::max(bmin.y, bmax.y - tsz.y);
                    tp = ImVec2(bmax.x - tsz.x * 0.5f, std::clamp(fmin.y - tsz.y * 0.5f, bmin.y, maxb));
                } else if (side == side_top) {
                    float maxb = std::max(bmin.x, bmax.x - tsz.x);
                    tp = ImVec2(std::clamp(fmax.x - tsz.x * 0.5f, bmin.x, maxb), bmin.y - tsz.y * 0.5f);
                } else {
                    float maxb = std::max(bmin.x, bmax.x - tsz.x);
                    tp = ImVec2(std::clamp(fmax.x - tsz.x * 0.5f, bmin.x, maxb), bmax.y - tsz.y * 0.5f);
                }

                AddTextOutlined(dl, font, font->LegacySize, tp, IM_COL32(255, 255, 255, (int)(255 * alpha)), val_str.c_str());
            }
        }
    }

    void RenderLayoutElements(const std::string& prefix, ImDrawList* dl, const ImVec2& box_min, const ImVec2& box_max, const std::string& player_name, const std::string& weapon_name, const CMisc::Panorama_t::IconData_t& icon, float health, float ammo, float max_ammo, float armor, float alpha, const std::vector<esp_flags::RenderedFlag_t>& flags_list, const std::vector<EspGrenadeInfo_t>& grenades = {}, bool is_reloading = false) {
        const int grp = (prefix.find("FRIENDLY") != std::string::npos || prefix.find("Friendly") != std::string::npos) ? 1 :
            ((prefix.find("LOCAL") != std::string::npos || prefix.find("Local") != std::string::npos) ? 2 : 0);

#define CFG_GET(type, var) (grp == 1 ? Config::Get<type>(Vars::Player::Friendly::var) : (grp == 2 ? Config::Get<type>(Vars::Player::Local::var) : Config::Get<type>(Vars::Player::Enemy::var)))

        const bool name_on = CFG_GET(bool, Name);
        const bool hb_on = CFG_GET(bool, HealthBar);
        const bool ammo_on = CFG_GET(bool, AmmoBar) && max_ammo > 0.f;
        const bool armor_on = CFG_GET(bool, ArmorBar) && armor > 0.f;
        const bool wname_on = CFG_GET(bool, WeaponName) && !weapon_name.empty();
        const bool wicon_on = CFG_GET(bool, WeaponIcon) && icon.texture_view;
        const bool gicons_on = CFG_GET(bool, GrenadeIcons) && !grenades.empty();

        ImFont* name_font = PickFont(CFG_GET(int, NameFont));
        ImFont* wname_font = PickFont(CFG_GET(int, WeaponNameFont));
        ImFont* hb_font = PickFont(CFG_GET(int, HealthBarFont));
        ImFont* ab_font = PickFont(CFG_GET(int, AmmoBarFont));
        ImFont* arb_font = PickFont(CFG_GET(int, ArmorBarFont));

        std::vector<Item_t> items;
        items.reserve(12);

        if (name_on)
            items.push_back({ 0, CFG_GET(int, LayoutNameSide), CFG_GET(int, LayoutNameAlign), CFG_GET(int, LayoutNameOrder), MeasureText(name_font, player_name), {} });

        if (hb_on)
            items.push_back({ 2, CFG_GET(int, LayoutHBarSide), CFG_GET(int, LayoutHBarAlign), CFG_GET(int, LayoutHBarOrder), MeasureBarDims(hb_font, health, 100.f, CFG_GET(int, HealthBarText)), {} });

        if (ammo_on)
            items.push_back({ 1, CFG_GET(int, LayoutABarSide), CFG_GET(int, LayoutABarAlign), CFG_GET(int, LayoutABarOrder), MeasureBarDims(ab_font, ammo, max_ammo, CFG_GET(int, AmmoBarText)), {} });

        if (armor_on)
            items.push_back({ 5, CFG_GET(int, LayoutArBarSide), CFG_GET(int, LayoutArBarAlign), CFG_GET(int, LayoutArBarOrder), MeasureBarDims(arb_font, armor, 100.f, CFG_GET(int, ArmorBarText)), {} });

        if (wname_on)
            items.push_back({ 3, CFG_GET(int, LayoutWNameSide), CFG_GET(int, LayoutWNameAlign), CFG_GET(int, LayoutWNameOrder), MeasureText(wname_font, weapon_name), {} });

        if (wicon_on)
            items.push_back({ 4, CFG_GET(int, LayoutWIconSide), CFG_GET(int, LayoutWIconAlign), CFG_GET(int, LayoutWIconOrder), MeasureIcon(icon), {} });

        if (gicons_on)
            items.push_back({ 6, CFG_GET(int, LayoutGrenadeIconsSide), CFG_GET(int, LayoutGrenadeIconsAlign), CFG_GET(int, LayoutGrenadeIconsOrder), MeasureGrenadeIconsDims(grenades), {} });

        for (auto& rf : flags_list) {
            ImFont* rf_font = PickFont(esp_flags::FlagFont(grp, rf.id));
            items.push_back({ 7 + rf.id, esp_flags::FlagSide(grp, rf.id), esp_flags::FlagAlign(grp, rf.id), esp_flags::FlagOrder(grp, rf.id), esp_flags::MeasureFlagDims(rf_font, rf.text), {} });
        }

        ComputeLayout(items, box_min, box_max);

        for (auto& it : items) {
            if (it.lid == 0) {
                Color_t nc = CFG_GET(Color_t, NameColor);
                ImVec2 draw_pos = it.rect.min;
                Dims_t text_dims = MeasureText(name_font, player_name);
                draw_pos.y += ((it.rect.max.y - it.rect.min.y) - text_dims.height) * 0.5f;

                AddTextOutlined(dl, name_font, name_font->LegacySize, draw_pos, IM_COL32((int)(nc.Rf() * 255.f), (int)(nc.Gf() * 255.f), (int)(nc.Bf() * 255.f), (int)(nc.Af() * 255.f * alpha)), player_name.c_str());
            } else if (it.lid == 2) {
                Color_t hc = CFG_GET(Color_t, HealthBarColor);
                Color_t hgc = CFG_GET(Color_t, HealthBarGradientColor);
                DrawBar(dl, it.rect, it.side, health, 100.f, alpha, CFG_GET(bool, HealthBarGlow), CFG_GET(bool, HealthBarGradient), hc, hgc, hb_font, CFG_GET(int, HealthBarText), false, false);
            } else if (it.lid == 1) {
                Color_t ac = CFG_GET(Color_t, AmmoBarColor);
                Color_t agc = CFG_GET(Color_t, AmmoBarGradientColor);
                DrawBar(dl, it.rect, it.side, ammo, max_ammo, alpha, CFG_GET(bool, AmmoBarGlow), CFG_GET(bool, AmmoBarGradient), ac, agc, ab_font, CFG_GET(int, AmmoBarText), true, is_reloading);
            } else if (it.lid == 5) {
                Color_t arc = CFG_GET(Color_t, ArmorBarColor);
                Color_t argc = CFG_GET(Color_t, ArmorBarGradientColor);
                DrawBar(dl, it.rect, it.side, armor, 100.f, alpha, CFG_GET(bool, ArmorBarGlow), CFG_GET(bool, ArmorBarGradient), arc, argc, arb_font, CFG_GET(int, ArmorBarText), false, false);
            } else if (it.lid == 3) {
                Color_t wc = CFG_GET(Color_t, WeaponNameColor);
                AddTextOutlined(dl, wname_font, wname_font->LegacySize, it.rect.min, IM_COL32((int)(wc.Rf() * 255.f), (int)(wc.Gf() * 255.f), (int)(wc.Bf() * 255.f), (int)(wc.Af() * 255.f * alpha)), weapon_name.c_str());
            } else if (it.lid == 4) {
                Color_t ic = CFG_GET(Color_t, WeaponIconColor);
                ImU32 icol = IM_COL32((int)(ic.Rf() * 255.f), (int)(ic.Gf() * 255.f), (int)(ic.Bf() * 255.f), (int)(ic.Af() * 255.f * alpha));
                if (icon.texture_view)
                    dl->AddImage((ImTextureID)icon.texture_view, it.rect.min, it.rect.max, ImVec2(0, 0), ImVec2(1, 1), icol);
            } else if (it.lid == 6) {
                Color_t gc = CFG_GET(Color_t, GrenadeIconsColor);
                float cur_x = it.rect.min.x;
                const float icon_h = it.rect.max.y - it.rect.min.y;
                const float gap = 3.f;

                for (const auto& g_info : grenades) {
                    float w = icon_h * 1.2f;
                    if (g_info.icon_data.width > 0 && g_info.icon_data.height > 0)
                        w = icon_h * ((float)g_info.icon_data.width / (float)g_info.icon_data.height);

                    float alpha_mult = g_info.is_active ? 1.0f : 0.5f;
                    int icon_alpha = static_cast<int>(gc.Af() * 255.f * alpha * alpha_mult);

                    ImU32 icol = IM_COL32((int)(gc.Rf() * 255.f), (int)(gc.Gf() * 255.f), (int)(gc.Bf() * 255.f), icon_alpha);
                    ImVec2 imin(cur_x, it.rect.min.y);
                    ImVec2 imax(cur_x + w, it.rect.max.y);

                    if (g_info.icon_data.texture_view)
                        dl->AddImage((ImTextureID)g_info.icon_data.texture_view, imin, imax, ImVec2(0, 0), ImVec2(1, 1), icol);

                    cur_x += w + gap;
                }
            } else if (it.lid >= 7) {
                int fid = it.lid - 7;

                for (auto& rf : flags_list) {
                    if (rf.id != fid)
                        continue;

                    ImFont* rf_font = PickFont(esp_flags::FlagFont(grp, fid));
                    int a = (rf.color >> IM_COL32_A_SHIFT) & 0xFF;
                    ImU32 col = IM_COL32((rf.color >> IM_COL32_R_SHIFT) & 0xFF, (rf.color >> IM_COL32_G_SHIFT) & 0xFF, (rf.color >> IM_COL32_B_SHIFT) & 0xFF, (int)(a * alpha));

                    AddTextOutlined(dl, rf_font, rf_font->LegacySize, it.rect.min, col, rf.text.c_str());
                    break;
                }
            }
        }

#undef CFG_GET
    }

    enum bone_indices_t : int {
        pelvis = 0,
        spine_1 = 2,
        spine_2 = 4,
        neck = 5,
        head = 6,
        arm_upper_l = 8,
        arm_lower_l = 9,
        hand_l = 10,
        arm_upper_r = 13,
        arm_lower_r = 14,
        hand_r = 15,
        leg_upper_l = 22,
        leg_lower_l = 23,
        ankle_l = 24,
        leg_upper_r = 25,
        leg_lower_r = 26,
        ankle_r = 27,
        bone_max = 128
    };

    static const std::vector<std::vector<int>> bone_chains = {
        { head, neck, spine_2, spine_1, pelvis },
        { spine_2, arm_upper_l, arm_lower_l, hand_l },
        { spine_2, arm_upper_r, arm_lower_r, hand_r },
        { pelvis, leg_upper_l, leg_lower_l, ankle_l },
        { pelvis, leg_upper_r, leg_lower_r, ankle_r }
    };

    static void DrawBoneChain(ImDrawList* dl, Vector* bone_positions, int bone_count, const std::vector<int>& chain, ImU32 col, float thickness = 1.5f) {
        for (size_t i = 0; i + 1 < chain.size(); i++) {
            int idx1 = chain[i];
            int idx2 = chain[i + 1];

            if (idx1 < 0 || idx1 >= bone_count || idx2 < 0 || idx2 >= bone_count)
                continue;

            Vector& pos1 = bone_positions[idx1];
            Vector& pos2 = bone_positions[idx2];

            if (pos1.Zero() || pos2.Zero())
                continue;

            Vector2D screen1 = Draw->WorldToScreen(pos1);
            Vector2D screen2 = Draw->WorldToScreen(pos2);

            if (screen1.Zero() || screen2.Zero() || !screen1.Valid() || !screen2.Valid())
                continue;

            if (!std::isfinite(screen1.x) || !std::isfinite(screen1.y) || !std::isfinite(screen2.x) || !std::isfinite(screen2.y))
                continue;

            float dx = screen1.x - screen2.x;
            float dy = screen1.y - screen2.y;
            if (dx * dx + dy * dy > 500.0f * 500.0f)
                continue;

            dl->AddLine(ImVec2(screen1.x, screen1.y), ImVec2(screen2.x, screen2.y), col, thickness);
        }
    }
}

bool Visuals_t::Player_t::GetBoundingBox(const Vector& origin, const Vector& mins, const Vector& maxs, ImVec2& out_min, ImVec2& out_max) {
    float left = std::numeric_limits<float>::max();
    float top = std::numeric_limits<float>::max();
    float right = std::numeric_limits<float>::lowest();
    float bottom = std::numeric_limits<float>::lowest();

    const ImVec2 display = ImGui::GetIO().DisplaySize;
    constexpr float edge_margin = 384.0f;
    bool had_valid_projection = false;

    for (int i = 0; i < 8; i++) {
        Vector point(i & 1 ? maxs.x : mins.x, i & 2 ? maxs.y : mins.y, i & 4 ? maxs.z : mins.z);
        Vector world_point = origin + point;

        Vector2D screen = Draw->WorldToScreen(world_point);
        if (screen.Zero() || !screen.Valid())
            continue;

        float sx = std::clamp(screen.x, -edge_margin, display.x + edge_margin);
        float sy = std::clamp(screen.y, -edge_margin, display.y + edge_margin);

        left = std::min(left, sx);
        top = std::min(top, sy);
        right = std::max(right, sx);
        bottom = std::max(bottom, sy);

        had_valid_projection = true;
    }

    if (!had_valid_projection || left >= right || top >= bottom)
        return false;

    out_min = ImVec2(left, top);
    out_max = ImVec2(right, bottom);
    return true;
}

void Visuals_t::Player_t::DrawSkeleton(const std::string& prefix, C_CSPlayerPawn* pawn, ImDrawList* dl) {
    if (!pawn || !dl)
        return;

    auto* node = pawn->m_pGameSceneNode();
    if (!node)
        return;

    auto* skel = node->GetSkeletonInstance();
    if (!skel || !skel->bone_cache || skel->bone_count <= 0)
        return;

    const int grp = (prefix.find("FRIENDLY") != std::string::npos || prefix.find("Friendly") != std::string::npos) ? 1 : ((prefix.find("LOCAL") != std::string::npos || prefix.find("Local") != std::string::npos) ? 2 : 0);

#define CFG_GET(type, var) (grp == 1 ? Config::Get<type>(Vars::Player::Friendly::var) : (grp == 2 ? Config::Get<type>(Vars::Player::Local::var) : Config::Get<type>(Vars::Player::Enemy::var)))
    Color_t col_cfg = CFG_GET(Color_t, SkeletonColor);
#undef CFG_GET

    ImU32 col = IM_COL32((int)(col_cfg.Rf() * 255.f), (int)(col_cfg.Gf() * 255.f), (int)(col_cfg.Bf() * 255.f), (int)(col_cfg.Af() * 255.f));

    Vector positions[128]{};
    int count = std::min(skel->bone_count, 128);

    Vector origin = node->m_vecAbsOrigin();
    bool is_local = false;

    if (count > 0) {
        Vector first_bone = skel->bone_cache[0].GetOrigin(0);
        if (first_bone.DistTo(origin) > 300.f && first_bone.Length() < 150.f)
            is_local = true;
    }

    for (int i = 0; i < count; i++) {
        positions[i] = skel->bone_cache[i].GetOrigin(0);
        if (is_local)
            positions[i] = origin + positions[i];
    }

    for (auto& chain : bone_chains)
        DrawBoneChain(dl, positions, count, chain, col);
}

void Visuals_t::Player_t::Render(const std::string& prefix, CCSPlayerController* controller, C_CSPlayerPawn* pawn, ImDrawList* draw_list) {
    if (!controller || !pawn || !pawn->Alive())
        return;

    C_CSWeaponBase* active_weapon = pawn->GetActiveWeapon();
    if (!active_weapon || !active_weapon->GetWeaponData())
        return;

    auto* collision = pawn->m_pCollision();
    auto* node = pawn->m_pGameSceneNode();
    if (!collision || !node)
        return;

    uintptr_t cache_id = reinterpret_cast<uintptr_t>(controller);
    float dt = ImGui::GetIO().DeltaTime;

    Vector origin = node->m_vecAbsOrigin();
    Vector mins = collision->m_vecMins();
    Vector maxs = collision->m_vecMaxs();

    ImVec2 box_min, box_max;
    if (!GetBoundingBox(origin, mins, maxs, box_min, box_max))
        return;

    box_min = RoundPos(box_min);
    box_max = RoundPos(box_max);

    const int grp = (prefix.find("FRIENDLY") != std::string::npos || prefix.find("Friendly") != std::string::npos) ? 1 : 
                    ((prefix.find("LOCAL") != std::string::npos || prefix.find("Local") != std::string::npos) ? 2 : 0);

#define CFG_GET(type, var) (grp == 1 ? Config::Get<type>(Vars::Player::Friendly::var) : (grp == 2 ? Config::Get<type>(Vars::Player::Local::var) : Config::Get<type>(Vars::Player::Enemy::var)))

    bool is_fading = CFG_GET(bool, Fading);

    float health = static_cast<float>(pawn->m_iHealth());
    float armor = static_cast<float>(pawn->m_ArmorValue());
    float ammo = 0.f;
    float max_ammo = 0.f;
    bool is_reloading = false;

    std::string player_name = SafeStr(controller->m_sSanitizedPlayerName());
    std::string weapon_name;
    uint16_t weapon_def_index = 0;

    auto get_def_index = [](C_CSWeaponBase* w) -> uint16_t {
        if (!w || !w->m_AttributeManager() || !w->m_AttributeManager()->m_Item())
            return 0;
        return w->m_AttributeManager()->m_Item()->m_iItemDefinitionIndex();
    };

    auto is_grenade = [](uint16_t idx) -> bool {
        switch (idx) {
        case 43: case 44: case 45: case 46: case 47: case 48:
            return true;
        default:
            return false;
        }
    };

    auto is_c4 = [](uint16_t idx) -> bool { return idx == 49; };
    auto is_utility = [&](uint16_t idx) -> bool { return is_grenade(idx) || is_c4(idx); };
    auto is_primary = [](int type) -> bool { return type >= 2 && type <= 6; };
    auto is_secondary = [](int type) -> bool { return type == 1; };

    C_CSWeaponBase* display_weapon = active_weapon;
    uint16_t active_idx = get_def_index(display_weapon);
    bool active_is_grenade = is_grenade(active_idx);

    if (active_weapon) {
        int real_clip = active_weapon->m_iClip1();
        int reserve = active_weapon->m_pReserveAmmo();

        if (auto wd = active_weapon->GetWeaponData())
            max_ammo = static_cast<float>(wd->m_iMaxClip1());

        is_reloading = (real_clip == 0 && reserve > 0 && max_ammo > 0.f);

        auto& anim = ammo_anims[cache_id];

        if (anim.visual_ammo < 0.f)
            anim.visual_ammo = static_cast<float>(real_clip);

        if (is_reloading) {
            if (!anim.was_reloading) {
                anim.reload_timer = 0.f;
                anim.was_reloading = true;
            }

            anim.reload_timer += dt * 0.45f;
            float progress = std::clamp(anim.reload_timer, 0.f, 1.f);
            float ease_progress = 1.f - std::pow(1.f - progress, 3.f);
            anim.visual_ammo = ease_progress * max_ammo;
        } else {
            anim.was_reloading = false;
            anim.reload_timer = 0.f;
            anim.visual_ammo = ImLerp(anim.visual_ammo, static_cast<float>(real_clip), std::clamp(dt * 18.f, 0.f, 1.f));
        }

        ammo = anim.visual_ammo;
    }

    if (!active_is_grenade && !is_c4(active_idx) && display_weapon)
        last_non_grenade_weapon[cache_id] = display_weapon->GetHandle();

    if (active_is_grenade) {
        auto it = last_non_grenade_weapon.find(cache_id);
        if (it != last_non_grenade_weapon.end() && it->second.Valid()) {
            C_CSWeaponBase* cached_wpn = it->second.get<C_CSWeaponBase>();
            if (cached_wpn)
                display_weapon = cached_wpn;
        }

        if (!display_weapon || is_utility(get_def_index(display_weapon))) {
            C_CSWeaponBase* primary = nullptr;
            C_CSWeaponBase* secondary = nullptr;
            C_CSWeaponBase* fallback = nullptr;

            auto* ws = pawn->m_pWeaponServices();
            if (ws) {
                for (int i = 0; i < ws->m_hMyWeapons().size; i++) {
                    if (!ws->m_hMyWeapons().elements[i].Valid())
                        continue;

                    C_CSWeaponBase* w = ws->m_hMyWeapons().elements[i].get<C_CSWeaponBase>();
                    if (!w)
                        continue;

                    uint16_t idx = get_def_index(w);
                    if (is_utility(idx))
                        continue;

                    auto wd = w->GetWeaponData();
                    if (!wd)
                        continue;

                    int wt = wd->m_WeaponType();
                    if (!primary && is_primary(wt))
                        primary = w;
                    if (!secondary && is_secondary(wt))
                        secondary = w;
                    if (!fallback)
                        fallback = w;
                }
            }

            if (primary)
                display_weapon = primary;
            else if (secondary)
                display_weapon = secondary;
            else if (fallback)
                display_weapon = fallback;
        }
    }

    if (display_weapon) {
        auto wd = display_weapon->GetWeaponData();
        if (wd) {
            std::string full_name = SafeStr(wd->m_szName());
            if (full_name.size() >= 7 && full_name.substr(0, 7) == xs("weapon_"))
                full_name.erase(0, 7);

            std::transform(full_name.begin(), full_name.end(), full_name.begin(), [](unsigned char c) { return (char)std::tolower(c); });
            std::replace(full_name.begin(), full_name.end(), '_', ' ');
            weapon_name = full_name;
        }

        weapon_def_index = get_def_index(display_weapon);
    }

    if (is_fading) {
        auto& cache = cached_players[cache_id];

        cache.is_dead = false;
        cache.health = health;
        cache.ammo = ammo;
        cache.max_ammo = max_ammo;
        cache.armor = armor;
        cache.name = player_name;
        cache.box_min = box_min;
        cache.box_max = box_max;
        cache.origin = origin;
        cache.mins = mins;
        cache.maxs = maxs;
        cache.weapon_name = weapon_name;
        cache.def_index = weapon_def_index;
    }

    if (CFG_GET(bool, Box)) {
        Color_t col = CFG_GET(Color_t, BoxColor);
        Color_t grad_col = CFG_GET(Color_t, BoxGradientColor);
        ImU32 col32 = IM_COL32((int)(col.Rf() * 255.f), (int)(col.Gf() * 255.f), (int)(col.Bf() * 255.f), (int)(col.Af() * 255.f));
        ImU32 grad32 = IM_COL32((int)(grad_col.Rf() * 255.f), (int)(grad_col.Gf() * 255.f), (int)(grad_col.Bf() * 255.f), (int)(grad_col.Af() * 255.f));
        ImU32 shadow32 = IM_COL32(0, 0, 0, 140);

        if (CFG_GET(bool, BoxGlow))
            AddBoxGlow(draw_list, box_min, box_max, col32);

        Draw->Rect(draw_list, ImVec2(box_min.x - 1.f, box_min.y - 1.f), ImVec2(box_max.x + 1.f, box_max.y + 1.f), shadow32, 0.f, 0, 1.f);
        Draw->Rect(draw_list, ImVec2(box_min.x + 1.f, box_min.y + 1.f), ImVec2(box_max.x - 1.f, box_max.y - 1.f), shadow32, 0.f, 0, 1.f);
        Draw->Rect(draw_list, box_min, box_max, col32, 0.f, 0, 1.f);
    }

    CMisc::Panorama_t::IconData_t weapon_icon_data{};
    if (CFG_GET(bool, WeaponIcon)) {
        if (weapon_def_index > 0)
            weapon_icon_data = Misc->Panorama.GetWeaponIcon(weapon_def_index);

        if (!weapon_icon_data.texture_view && display_weapon && display_weapon->GetWeaponData()) {
            const char* sz_name = display_weapon->GetWeaponData()->m_szName();
            if (sz_name)
                weapon_icon_data = Misc->Panorama.GetWeaponIconByName(sz_name);
        }

        if (!weapon_icon_data.texture_view && !weapon_name.empty()) {
            std::string weapon_key = xs("weapon_") + weapon_name;
            std::replace(weapon_key.begin(), weapon_key.end(), ' ', '_');
            weapon_icon_data = Misc->Panorama.GetWeaponIconByName(weapon_key.c_str());
        }
    }

    esp_flags::Input_t flags_input = GatherFlagsInput(controller, pawn, active_weapon, active_is_grenade);
    esp_flags::State_t& flag_state = flags_state[cache_id];
    esp_flags::UpdateState(prefix, flag_state, flags_input, dt);
    std::vector<esp_flags::RenderedFlag_t> flags_list = esp_flags::BuildRenderList(prefix, flag_state, flags_input);

    std::vector<EspGrenadeInfo_t> player_grenades;
    if (CFG_GET(bool, GrenadeIcons))
        player_grenades = GatherPlayerGrenades(pawn, active_weapon);

#undef CFG_GET

    RenderLayoutElements(prefix, draw_list, box_min, box_max, player_name, weapon_name, weapon_icon_data, health, ammo, max_ammo, armor, 1.f, flags_list, player_grenades, is_reloading);
}

void Visuals_t::Player_t::RenderCached(const std::string& prefix, uintptr_t cache_id, CachedPlayerData_t& cache, float alpha, ImDrawList* draw_list) {
    ImVec2 box_min, box_max;
    if (!GetBoundingBox(cache.origin, cache.mins, cache.maxs, box_min, box_max))
        return;

    box_min = RoundPos(box_min);
    box_max = RoundPos(box_max);

    cache.box_min = box_min;
    cache.box_max = box_max;

    float displayed_health = cache.health * alpha;
    float displayed_ammo = cache.ammo;
    float displayed_armor = cache.armor;

    const int grp = (prefix.find("FRIENDLY") != std::string::npos || prefix.find("Friendly") != std::string::npos) ? 1 :  ((prefix.find("LOCAL") != std::string::npos || prefix.find("Local") != std::string::npos) ? 2 : 0);

#define CFG_GET(type, var) (grp == 1 ? Config::Get<type>(Vars::Player::Friendly::var) : (grp == 2 ? Config::Get<type>(Vars::Player::Local::var) : Config::Get<type>(Vars::Player::Enemy::var)))

    if (CFG_GET(bool, Box)) {
        Color_t col = CFG_GET(Color_t, BoxColor);
        Color_t grad_col = CFG_GET(Color_t, BoxGradientColor);
        ImU32 col32 = IM_COL32((int)(col.Rf() * 255.f), (int)(col.Gf() * 255.f), (int)(col.Bf() * 255.f), static_cast<int>(col.Af() * 255.f * alpha));
        ImU32 grad32 = IM_COL32((int)(grad_col.Rf() * 255.f), (int)(grad_col.Gf() * 255.f), (int)(grad_col.Bf() * 255.f), static_cast<int>(grad_col.Af() * 255.f * alpha));
        ImU32 shadow32 = IM_COL32(0, 0, 0, static_cast<int>(140 * alpha));

        if (CFG_GET(bool, BoxGlow))
            AddBoxGlow(draw_list, box_min, box_max, col32);

        Draw->Rect(draw_list, ImVec2(box_min.x - 1.f, box_min.y - 1.f), ImVec2(box_max.x + 1.f, box_max.y + 1.f), shadow32, 0.f, 0, 1.f);
        Draw->Rect(draw_list, ImVec2(box_min.x + 1.f, box_min.y + 1.f), ImVec2(box_max.x - 1.f, box_max.y - 1.f), shadow32, 0.f, 0, 1.f);
        Draw->Rect(draw_list, box_min, box_max, col32, 0.f, 0, 1.f);
    }

    CMisc::Panorama_t::IconData_t weapon_icon_data{};
    if (CFG_GET(bool, WeaponIcon)) {
        if (cache.def_index > 0)
            weapon_icon_data = Misc->Panorama.GetWeaponIcon(cache.def_index);

        if (!weapon_icon_data.texture_view && !cache.weapon_name.empty()) {
            std::string weapon_key = xs("weapon_") + cache.weapon_name;
            std::replace(weapon_key.begin(), weapon_key.end(), ' ', '_');
            weapon_icon_data = Misc->Panorama.GetWeaponIconByName(weapon_key.c_str());
        }
    }

#undef CFG_GET

    RenderLayoutElements(prefix, draw_list, box_min, box_max, cache.name, cache.weapon_name, weapon_icon_data, displayed_health, displayed_ammo, cache.max_ammo, displayed_armor, alpha, {}, {}, false);
}

void Visuals_t::Player_t::OnPresent() {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    if (!draw_list)
        return;

    C_CSPlayerPawn* local_pawn = ctx.pawn;
    if (!local_pawn)
        return;

    const float current_time = static_cast<float>(ImGui::GetTime());
    std::unordered_set<uintptr_t> active_alive_ids;

    if (!ctx.controller || !GlobalVars)
        return;

    if (GlobalVars && GlobalVars->map_name_short && std::string(GlobalVars->map_name_short).contains("<empty>"))
        return;

    for (auto& player : EntityList->players) {
        if (!player.Fill())
            continue;

        CCSPlayerController* controller = player.controller;
        C_CSPlayerPawn* pawn = player.pawn;

        if (!controller || !pawn)
            continue;

        std::string prefix;
        bool is_enabled = false;
        bool skeleton_enabled = false;

        if (pawn == local_pawn) {
            if (Config::Get<bool>(Vars::World::Thirdperson)) {
                prefix = xs("PLAYER.LOCAL.");
                is_enabled = Config::Get<bool>(Vars::Player::Local::Enable);
                skeleton_enabled = Config::Get<bool>(Vars::Player::Local::Skeleton);
            } else {
                continue;
            }
        } else if (pawn->Enemy(local_pawn)) {
            prefix = xs("PLAYER.ENEMY.");
            is_enabled = Config::Get<bool>(Vars::Player::Enemy::Enable);
            skeleton_enabled = Config::Get<bool>(Vars::Player::Enemy::Skeleton);
        } else {
            prefix = xs("PLAYER.FRIENDLY.");
            is_enabled = Config::Get<bool>(Vars::Player::Friendly::Enable);
            skeleton_enabled = Config::Get<bool>(Vars::Player::Friendly::Skeleton);
        }

        if (!is_enabled)
            continue;

        uintptr_t cache_id = reinterpret_cast<uintptr_t>(controller);

        if (pawn->Alive()) {
            active_alive_ids.insert(cache_id);
            this->Render(prefix, controller, pawn, draw_list);

            if (skeleton_enabled)
                this->DrawSkeleton(prefix, pawn, draw_list);
        }
    }

    struct GroupData {
        std::string prefix;
        bool fading;
        float duration;
    };

    const GroupData groups[] = {
        { xs("PLAYER.ENEMY."), Config::Get<bool>(Vars::Player::Enemy::Fading), Config::Get<float>(Vars::Player::Enemy::FadingDuration) },
        { xs("PLAYER.FRIENDLY."), Config::Get<bool>(Vars::Player::Friendly::Fading), Config::Get<float>(Vars::Player::Friendly::FadingDuration) },
        { xs("PLAYER.LOCAL."), Config::Get<bool>(Vars::Player::Local::Fading), Config::Get<float>(Vars::Player::Local::FadingDuration) }
    };

    for (const auto& grp_data : groups) {
        if (!grp_data.fading)
            continue;

        const float fade_duration = grp_data.duration;

        for (auto it = this->cached_players.begin(); it != this->cached_players.end(); ) {
            uintptr_t cache_id = it->first;
            auto& cache = it->second;

            if (active_alive_ids.find(cache_id) == active_alive_ids.end()) {
                if (!cache.is_dead) {
                    cache.is_dead = true;
                    cache.death_time = current_time;
                }
            }

            if (cache.is_dead) {
                float elapsed = current_time - cache.death_time;

                if (fade_duration <= 0.0f || elapsed >= fade_duration) {
                    flags_state.erase(cache_id);
                    ammo_anims.erase(cache_id);
                    it = this->cached_players.erase(it);
                    continue;
                }

                float alpha = std::clamp(1.0f - (elapsed / fade_duration), 0.0f, 1.0f);
                this->RenderCached(grp_data.prefix, cache_id, cache, alpha, draw_list);
            }

            ++it;
        }
    }
}