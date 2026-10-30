#include <shared.h>

extern int GetActiveSubtab(int);

void CGui::RenderVisualsContent() {
    const int subtab = GetActiveSubtab(2);

    std::string prefix;
    switch (this->esp_target) {
    case 0: prefix = xs("PLAYER.ENEMY."); break;
    case 1: prefix = xs("PLAYER.FRIENDLY."); break;
    case 2: prefix = xs("PLAYER.LOCAL."); break;
    }

    if (subtab == 0) {
        this->BeginGroup();
        {
            this->BeginChild(xs("Player ESP"), ICON_FA_USER);
            {
                this->Dropdown(xs("Target"), &this->esp_target, { xs("Enemy"), xs("Team"), xs("Local") });

                bool* p_enable = (this->esp_target == 0 ? &Config::Get<bool>(Vars::Player::Enemy::Enable) : (this->esp_target == 1 ? &Config::Get<bool>(Vars::Player::Friendly::Enable) : &Config::Get<bool>(Vars::Player::Local::Enable)));
                this->Checkbox(xs("Enable"), p_enable);

                if (this->esp_target != 2) {
                    bool* p_fading = (this->esp_target == 0 ? &Config::Get<bool>(Vars::Player::Enemy::Fading) : &Config::Get<bool>(Vars::Player::Friendly::Fading));
                    float* p_fading_dur = (this->esp_target == 0 ? &Config::Get<float>(Vars::Player::Enemy::FadingDuration) : &Config::Get<float>(Vars::Player::Friendly::FadingDuration));

                    this->CheckboxEx(xs("Fading"), p_fading, false, nullptr, 0, [this, p_fading_dur]() { 
                        this->SliderFloat(xs("Duration"), p_fading_dur, 0.1f, 2.f, xs("%.1fs")); 
                    });
                }
            }
            this->EndChild();

            this->BeginChild(xs("Colored Models"), ICON_FA_FILL);
            {
                auto render_settings = [this](uint32_t var_enable, uint32_t var_color, uint32_t var_overlay, uint32_t var_mat, uint32_t var_exp, uint32_t var_fall) {
                    static std::vector<std::string> mats = { xs("Shaded"), xs("Solid"), xs("Glow"), xs("Glossy") };

                    Color_t* clr_ptr[1] = { &Config::Get<Color_t>(var_color) };
                    this->CheckboxEx(xs("Enable"), &Config::Get<bool>(var_enable), false, clr_ptr, 1);

                    if (Config::Get<bool>(var_enable)) {
                        this->Checkbox(xs("Overlay"), &Config::Get<bool>(var_overlay));
                        this->Dropdown(xs("Material"), &Config::Get<int>(var_mat), mats);

                        int mat_id = Config::Get<int>(var_mat);
                        if (mat_id == 2 || mat_id == 3) {
                            this->SliderFloat(xs("Exponent"), &Config::Get<float>(var_exp), 0.f, 5.f, xs("%.f%%"), 20.f);
                            this->SliderFloat(xs("Falloff"), &Config::Get<float>(var_fall), 0.f, 20.f, xs("%.f%%"), 5.f);
                        }
                    }
                };

#define CALL_RENDER_SETTINGS(group) \
    render_settings(Vars::ColoredModels::group::Enable, Vars::ColoredModels::group::Color, Vars::ColoredModels::group::Overlay, Vars::ColoredModels::group::Material, Vars::ColoredModels::group::Exponent, Vars::ColoredModels::group::Falloff)

                if (this->esp_target == 0) {
                    static int enemy_mat_cat = 0;
                    this->Dropdown(xs("Category"), &enemy_mat_cat, { xs("Visible"), xs("Occluded") });
                    if (enemy_mat_cat == 0)
                        CALL_RENDER_SETTINGS(EnemyVisible);
                    else
                        CALL_RENDER_SETTINGS(EnemyOccluded);
  
                    Color_t* soul_clr[1]{ &Config::Get<Color_t>(Vars::Effects::SoulColor) };
                    this->CheckboxEx(xs("Soul Effect"), &Config::Get<bool>(Vars::Effects::Soul), false, soul_clr, 1, [this]() {
                        this->Checkbox(xs("Show Behind Wall"), &Config::Get<bool>(Vars::Effects::SoulZ));
                        this->SliderFloat(xs("Duration"), &Config::Get<float>(Vars::Effects::SoulDuration), 0.1f, 10.f, xs("%.1fs"));
                        this->SliderFloat(xs("Density"), &Config::Get<float>(Vars::Effects::SoulDensity), 100.f, 1000.f, xs("%.f%%"), 0.1f);
                        this->SliderFloat(xs("Intensity"), &Config::Get<float>(Vars::Effects::SoulIntensity), 0.1f, 5.f, xs("%.1fpx"));
                        this->SliderFloat(xs("Glow Intensity"), &Config::Get<float>(Vars::Effects::SoulGlowIntensity), 0.f, 5.f, xs("%.1fpx"));

                        Vector gravity = Vector(Config::Get<float>(Vars::Effects::SoulGravityX), Config::Get<float>(Vars::Effects::SoulGravityY), Config::Get<float>(Vars::Effects::SoulGravityZ));
                        if (this->VectorScalar(xs("Direction"), &gravity, -100.f, 100.f, -100.f, 100.f, -100.f, 100.f)) {
                            Config::Set<float>(Vars::Effects::SoulGravityX, gravity.x);
                            Config::Set<float>(Vars::Effects::SoulGravityY, gravity.y);
                            Config::Set<float>(Vars::Effects::SoulGravityZ, gravity.z);
                        }
                    });
                } else if (this->esp_target == 1) {
                    static int friendly_mat_cat = 0;
                    this->Dropdown(xs("Category"), &friendly_mat_cat, { xs("Visible"), xs("Occluded") });
                    if (friendly_mat_cat == 0)
                        CALL_RENDER_SETTINGS(FriendlyVisible);
                    else
                        CALL_RENDER_SETTINGS(FriendlyOccluded);
                } else if (this->esp_target == 2) {
                    static int local_mat_cat = 0;
                    this->Dropdown(xs("Category"), &local_mat_cat, { xs("Local"), xs("Arms"), xs("Weapon"), xs("Attachments") });
                    switch (local_mat_cat) {
                    case 0: CALL_RENDER_SETTINGS(Local); break;
                    case 1: CALL_RENDER_SETTINGS(Arms); break;
                    case 2: CALL_RENDER_SETTINGS(Weapon); break;
                    case 3: CALL_RENDER_SETTINGS(Attachments); break;
                    }

                    this->CheckboxEx(xs("Scope Blend"), &Config::Get<bool>(Vars::ColoredModels::ScopeBlend), false, nullptr, 0, [this]() {
                        this->SliderInt(xs("Alpha"), &Config::Get<int>(Vars::ColoredModels::ScopeBlendValue), 0, 255);
                        this->Checkbox(xs("Keep On Resume"), &Config::Get<bool>(Vars::ColoredModels::ScopeBlendKeepOnResume));
                    });
                }

#undef CALL_RENDER_SETTINGS
            }
            this->EndChild();

            this->BeginChild(xs("Effects"), ICON_FA_EXPLOSION);
            {
                if (this->esp_target == 0) {
                    Color_t* enemy_clr[1]{ &Config::Get<Color_t>(Vars::Effects::BeamsEnemyColor) };
                    this->CheckboxEx(xs("Enable Enemy Beams"), &Config::Get<bool>(Vars::Effects::BeamsEnemy), false, enemy_clr, 1, [this]() {
                        this->SliderInt(xs("Duration"), &Config::Get<int>(Vars::Effects::BeamsEnemyDuration), 1, 10, xs("%ds"));
                        this->SliderFloat(xs("Width"), &Config::Get<float>(Vars::Effects::BeamsEnemyWidth), 0.1f, 10.f, xs("%.f%%"), 10.f);
                    });
                } else if (this->esp_target == 1) {
                    Color_t* friendly_clr[1]{ &Config::Get<Color_t>(Vars::Effects::BeamsFriendlyColor) };
                    this->CheckboxEx(xs("Enable Team Beams"), &Config::Get<bool>(Vars::Effects::BeamsFriendly), false, friendly_clr, 1, [this]() {
                        this->SliderInt(xs("Duration"), &Config::Get<int>(Vars::Effects::BeamsFriendlyDuration), 1, 10, xs("%ds"));
                        this->SliderFloat(xs("Width"), &Config::Get<float>(Vars::Effects::BeamsFriendlyWidth), 0.1f, 10.f, xs("%.f%%"), 10.f);
                    });
                } else if (this->esp_target == 2) {
                    Color_t* local_clr[1]{ &Config::Get<Color_t>(Vars::Effects::BeamsLocalColor) };
                    this->CheckboxEx(xs("Enable Local Beams"), &Config::Get<bool>(Vars::Effects::BeamsLocal), false, local_clr, 1, [this]() {
                        this->SliderInt(xs("Duration"), &Config::Get<int>(Vars::Effects::BeamsLocalDuration), 1, 10, xs("%ds"));
                        this->SliderFloat(xs("Width"), &Config::Get<float>(Vars::Effects::BeamsLocalWidth), 0.1f, 10.f, xs("%.f%%"), 10.f);
                    });

                    this->ListEx(xs("Bullet Impacts"), [this]() {
                        Color_t* cl_clr[1]{ &Config::Get<Color_t>(Vars::Effects::ImpactsClientColor) };
                        this->CheckboxEx(xs("Enable Client"), &Config::Get<bool>(Vars::Effects::ImpactsClient), false, cl_clr, 1, [this]() {
                            this->SliderFloat(xs("Size"), &Config::Get<float>(Vars::Effects::ImpactsClientSize), 1.f, 10.f, xs("%.fft"));
                            this->SliderFloat(xs("Duration"), &Config::Get<float>(Vars::Effects::ImpactsClientDuration), 1.f, 10.f, xs("%.fs"));
                        });

                        Color_t* sv_clr[1]{ &Config::Get<Color_t>(Vars::Effects::ImpactsServerColor) };
                        this->CheckboxEx(xs("Enable Server"), &Config::Get<bool>(Vars::Effects::ImpactsServer), false, sv_clr, 1, [this]() {
                            this->SliderFloat(xs("Size"), &Config::Get<float>(Vars::Effects::ImpactsServerSize), 1.f, 10.f, xs("%.fft"));
                            this->SliderFloat(xs("Duration"), &Config::Get<float>(Vars::Effects::ImpactsServerDuration), 1.f, 10.f, xs("%.fs"));
                        });
                    });

                    Color_t* sparks_c[1]{ &Config::Get<Color_t>(Vars::Effects::ShotSparksColor) };
                    this->CheckboxEx(xs("Shot Sparks"), &Config::Get<bool>(Vars::Effects::ShotSparks), false, sparks_c, 1, [this]() {
                        this->SliderFloat(xs("Density"), &Config::Get<float>(Vars::Effects::ShotSparksDensity), 100.f, 1000.f, xs("%.f%%"), 0.1f);
                        this->SliderFloat(xs("Intensity"), &Config::Get<float>(Vars::Effects::ShotSparksIntensity), 0.1f, 5.f, xs("%.1fpx"));
                        this->SliderFloat(xs("Glow Intensity"), &Config::Get<float>(Vars::Effects::ShotSparksGlowIntensity), 0.f, 5.f, xs("%.1fpx"));
                        this->SliderFloat(xs("Size"), &Config::Get<float>(Vars::Effects::ShotSparksSize), 0.1f, 1.f, xs("%.1fpx"));
                        this->SliderFloat(xs("Length"), &Config::Get<float>(Vars::Effects::ShotSparksLength), 0.01f, 1.f, xs("%.2fpx"));

                        Vector gravity = Vector(0.f, 0.f, Config::Get<float>(Vars::Effects::ShotSparksGravityZ));
                        if (this->VectorScalar(xs("Direction"), &gravity, 0.f, 0.f, 0.f, 0.f, -1000.f, 1000.f))
                            Config::Set<float>(Vars::Effects::ShotSparksGravityZ, gravity.z);
                    });
                }
            }
            this->EndChild();
        }
        this->EndGroup();

        this->Sameline();

        this->BeginGroup();
        {
            this->BeginChild(xs("Preview"), ICON_FA_PERSON, ImVec2(0.f, 580.f));
            {
                this->RenderEspPreview(Var->gui.content_alpha, ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 38.f), prefix);
            }
            this->EndChild();
        }
        this->EndGroup();
    } else {
        this->BeginGroup();
        {
            this->BeginChild(xs("Grenades"), ICON_FA_USER);
            {
                Color_t* clrs[2] = { &Config::Get<Color_t>(Vars::Grenades::TrajectoryColor), &Config::Get<Color_t>(Vars::Grenades::TrajectoryHitColor) };
                Color_t* pclrs[2] = { &Config::Get<Color_t>(Vars::Grenades::PredictDamageColor1), &Config::Get<Color_t>(Vars::Grenades::PredictDamageColor2) };
                this->CheckboxEx(xs("Grenade Trajectory"), &Config::Get<bool>(Vars::Grenades::Trajectory), false, clrs, 2, [this, &pclrs]() {
                    this->Checkbox(xs("Pin Pulled Only"), &Config::Get<bool>(Vars::Grenades::TrajectoryPinPulled));
                    this->CheckboxEx(xs("Predict Damage"), &Config::Get<bool>(Vars::Grenades::PredictDamage), false, pclrs, 2, [this]() {
                        this->Dropdown(xs("Font"), &Config::Get<int>(Vars::Grenades::PredictDamageFont), { xs("Pixel"), xs("Bold"), xs("Default") });
                    });
                    this->SliderFloat(xs("Glow Intensity"), &Config::Get<float>(Vars::Grenades::TrajectoryGlowIntensity), 0.f, 1.f, xs("%.f%%"), 100.f);
                    this->SliderFloat(xs("Scale"), &Config::Get<float>(Vars::Grenades::TrajectoryScale), 0.f, 10.f, xs("%.f%%"), 10.f);
                });

                Color_t* wclr[1] = { &Config::Get<Color_t>(Vars::Grenades::WarningsColor) };
                this->CheckboxEx(xs("Grenade Warnings"), &Config::Get<bool>(Vars::Grenades::Warnings), false, wclr, 1);
            }
            this->EndChild();
        }
        this->EndGroup();
    }
}
