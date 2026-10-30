#include <shared.h>

extern int GetActiveSubtab(int);

void CGui::RenderWorldContent() {
	const int subtab = GetActiveSubtab(3);

	if (subtab == 0) {
		this->BeginGroup();
		{
			this->BeginChild(xs("Preview"));
			{

			}
			this->EndChild();
		}
		this->EndGroup();

		this->Sameline();

		this->BeginGroup();
		{
			this->BeginChild(xs("General"));
			{
				this->Checkbox(xs("Enable"), &Config::Get<bool>(Vars::World::Enable));

				Color_t* world_color[1] = { &Config::Get<Color_t>(Vars::World::WorldColor) };
				this->CheckboxEx(xs("Modulate World"), &Config::Get<bool>(Vars::World::ModulateWorld), false, world_color, 1);

				Color_t* light_color[1] = { &Config::Get<Color_t>(Vars::World::LightColor) };
				this->CheckboxEx(xs("Modulate Light"), &Config::Get<bool>(Vars::World::ModulateLight), false, light_color, 1);

				Color_t* skybox_color[1] = { &Config::Get<Color_t>(Vars::World::SkyboxColor) };
				this->CheckboxEx(xs("Modulate Skybox"), &Config::Get<bool>(Vars::World::ModulateSkybox), false, skybox_color, 1);

				Color_t* sun_color[1] = { &Config::Get<Color_t>(Vars::World::SunColor) };
				this->CheckboxEx(xs("Modulate Sun"), &Config::Get<bool>(Vars::World::ModulateSun), false, sun_color, 1);

				Color_t* clouds_color[1] = { &Config::Get<Color_t>(Vars::World::CloudsColor) };
				this->CheckboxEx(xs("Modulate Clouds"), &Config::Get<bool>(Vars::World::ModulateClouds), false, clouds_color, 1);

				this->CheckboxWithDropdown(xs("Weather"), &Config::Get<bool>(Vars::Effects::Weather), &Config::Get<int>(Vars::Effects::WeatherType), { xs("Stars"), xs("Snow"), xs("Ash") }, false, [this]() {
					if (Config::Get<int>(Vars::Effects::WeatherType) == 0) {
						Color_t* stars_clr[1]{ &Config::Get<Color_t>(Vars::Effects::StarsColor) };
						this->ColorEdit(xs("Color"), stars_clr, 1);
						
						this->SliderFloat(xs("Intensity"), &Config::Get<float>(Vars::Effects::StarsGlowIntensity), 0.f, 5.f, xs("%.1fpx"));
						this->SliderFloat(xs("Glow Intensity"), &Config::Get<float>(Vars::Effects::StarsIntensity), 0.1f, 5.f, xs("%.1fpx"));
						this->SliderFloat(xs("Density"), &Config::Get<float>(Vars::Effects::StarsDensity), 100.f, 1000.f, xs("%.f%%"), 0.1f);
						
						Vector gravity = Vector(Config::Get<float>(Vars::Effects::StarsGravityX), Config::Get<float>(Vars::Effects::StarsGravityY), Config::Get<float>(Vars::Effects::StarsGravityZ));
						if (this->VectorScalar(xs("Direction"), &gravity, -1000.f, 1000.f, -1000.f, 1000.f, -1000.f, 1000.f)) {
							Config::Set<float>(Vars::Effects::StarsGravityX, gravity.x);
							Config::Set<float>(Vars::Effects::StarsGravityY, gravity.y);
							Config::Set<float>(Vars::Effects::StarsGravityZ, gravity.z);
						}
					} else if (Config::Get<int>(Vars::Effects::WeatherType) == 1) {
						Color_t* snow_clr[1]{ &Config::Get<Color_t>(Vars::Effects::SnowColor) };
						this->ColorEdit(xs("Color"), snow_clr, 1);

						this->SliderFloat(xs("Size"), &Config::Get<float>(Vars::Effects::SnowSize), 0.1f, 1.f, xs("%.1fpx"));
						this->SliderFloat(xs("Glow Intensity"), &Config::Get<float>(Vars::Effects::SnowGlowIntensity), 0.f, 5.f, xs("%.1fpx"));
						this->SliderFloat(xs("Density"), &Config::Get<float>(Vars::Effects::SnowDensity), 100.f, 1000.f, xs("%.f%%"), 0.1f);
						this->SliderFloat(xs("Opacity"), &Config::Get<float>(Vars::Effects::SnowOpacity), 0.f, 1.f, xs("%.f%%"), 100.f);
						this->SliderFloat(xs("Glow Opacity"), &Config::Get<float>(Vars::Effects::SnowGlowOpacity), 0.f, 1.f, xs("%.f%%"), 100.f);

						Vector gravity = Vector(Config::Get<float>(Vars::Effects::SnowGravityX), Config::Get<float>(Vars::Effects::SnowGravityY), Config::Get<float>(Vars::Effects::SnowGravityZ));
						if (this->VectorScalar(xs("Direction"), &gravity, -1000.f, 1000.f, -1000.f, 1000.f, -1000.f, 1000.f)) {
							Config::Set<float>(Vars::Effects::SnowGravityX, gravity.x);
							Config::Set<float>(Vars::Effects::SnowGravityY, gravity.y);
							Config::Set<float>(Vars::Effects::SnowGravityZ, gravity.z);
						}
					} else if (Config::Get<int>(Vars::Effects::WeatherType) == 2) {
						Color_t* ash_clr[1]{ &Config::Get<Color_t>(Vars::Effects::AshColor) };
						this->ColorEdit(xs("Color"), ash_clr, 1);

						this->SliderFloat(xs("Duration"), &Config::Get<float>(Vars::Effects::AshDuration), 1.f, 10.f, xs("%.1fs"));
						this->SliderFloat(xs("Density"), &Config::Get<float>(Vars::Effects::AshDensity), 100.f, 1000.f, xs("%.f%%"), 0.1f);
						this->SliderFloat(xs("Ash Density"), &Config::Get<float>(Vars::Effects::AshChildDensity), 100.f, 1000.f, xs("%.f%%"), 0.1f);

						Vector gravity = Vector(Config::Get<float>(Vars::Effects::AshGravityX), Config::Get<float>(Vars::Effects::AshGravityY), Config::Get<float>(Vars::Effects::AshGravityZ));
						if (this->VectorScalar(xs("Direction"), &gravity, -1000.f, 1000.f, -1000.f, 1000.f, -1000.f, 1000.f)) {
							Config::Set<float>(Vars::Effects::AshGravityX, gravity.x);
							Config::Set<float>(Vars::Effects::AshGravityY, gravity.y);
							Config::Set<float>(Vars::Effects::AshGravityZ, gravity.z);
						}
					}
				});

				this->CheckboxWithDropdown(xs("Skybox"), &Config::Get<bool>(Vars::World::SkyboxChanger), &Config::Get<int>(Vars::World::SkyboxChangerType), { xs("Default"), xs("Sunset"), xs("Italy"), xs("Overcast"), xs("Night"), xs("Aztec"), xs("Jungle"), xs("Rain"), xs("Cloudy") });

				this->CheckboxEx(xs("Model Wetness"), &Config::Get<bool>(Vars::World::ModelWetness), false, nullptr, 0, [this]() {
					this->SliderFloat(xs("Coverage"), &Config::Get<float>(Vars::World::ModelWetnessCoverage), 0.f, 4.f, xs("%.f%%"), 25.f);
					this->SliderFloat(xs("Speed"), &Config::Get<float>(Vars::World::ModelWetnessSpeed), 0.f, 10.f, xs("%.f%%"), 10.f);
				});

				this->CheckboxEx(xs("Bloom"), &Config::Get<bool>(Vars::World::Bloom), false, nullptr, 0, [this]() {
					const char* fmt = Config::Get<float>(Vars::World::BloomIntensity) == 0.0f ? xs("Default") : xs("%.f%%");
					this->SliderFloat(xs("Intensity"), &Config::Get<float>(Vars::World::BloomIntensity), 0.f, 3.f, fmt, 100.f);
				});

				this->CheckboxEx(xs("Gamma"), &Config::Get<bool>(Vars::World::Gamma), false, nullptr, 0, [this]() {
					const char* fmt = (std::fabsf(Config::Get<float>(Vars::World::GammaValue) - 2.2f) < 0.01f) ? xs("Default") : xs("%.0f%%");
					this->SliderFloat(xs("Value"), &Config::Get<float>(Vars::World::GammaValue), 0.22f, 6.6f, fmt, 100.f / 2.2f);
				});

				this->CheckboxEx(xs("Depth Of Field"), &Config::Get<bool>(Vars::World::DepthOfField), false, nullptr, 0, [this]() {
					this->SliderFloat(xs("Near Distance"), &Config::Get<float>(Vars::World::DepthOfFieldNearDistance), 0.f, 1000.f, xs("%.fft"));
					this->SliderFloat(xs("Far Distance"), &Config::Get<float>(Vars::World::DepthOfFieldFarDistance), 100.f, 4000.f, xs("%.fft"));
				});

				Color_t* fullbright_color[1] = { &Config::Get<Color_t>(Vars::World::FullbrightColor) };
				this->CheckboxEx(xs("Fullbright"), &Config::Get<bool>(Vars::World::Fullbright), false, fullbright_color, 1);

				const char* fmt = Config::Get<float>(Vars::World::Exposure) == 0.0f || Config::Get<float>(Vars::World::Exposure) == 100.f ? xs("Default") : xs("%.f%%");
				this->SliderFloat(xs("Exposure"), &Config::Get<float>(Vars::World::Exposure), 0.f, 300.f, fmt);

				this->MultiDropdown(xs("Removals"), &Config::Get<std::vector<int>>(Vars::World::Removals), { xs("3D Skybox"), xs("2D Skybox"), xs("Lighting") });
			}
			this->EndChild();
		}
		this->EndGroup();
	} else {
		this->BeginGroup();
		{
			this->BeginChild(xs("General"));
			{
				this->CheckboxEx(xs("Thirdperson"), &Config::Get<bool>(Vars::World::Thirdperson), false, nullptr, 0, [this]() {
					this->Checkbox(xs("Disable Animation"), &Config::Get<bool>(Vars::World::ThirdpersonDisableAnimation));
					this->SliderInt(xs("Distance"), &Config::Get<int>(Vars::World::ThirdpersonDistance), 25, 150);
				});

				this->CheckboxEx(xs("Aspect Ratio"), &Config::Get<bool>(Vars::World::AspectRatio), false, nullptr, 0, [this]() {
					this->SliderFloat(xs("Value"), &Config::Get<float>(Vars::World::AspectRatioValue), 0.0f, 2.f, xs("%.2f"));
				});

				this->CheckboxEx(xs("Field Of View"), &Config::Get<bool>(Vars::World::FieldOfViewEnabled), false, nullptr, 0, [this]() {
					this->SliderFloat(xs("Value"), &Config::Get<float>(Vars::World::FieldOfView), 50.f, 130.f, xs("%.f°"));
					this->ListEx(xs("Override In-Scope"), [this]() {
						const char* fmt1 = Config::Get<float>(Vars::World::FieldOfViewFirstZoom) == 0.0f ? xs("Default") : xs("%.f%%");
						const char* fmt2 = Config::Get<float>(Vars::World::FieldOfViewSecondZoom) == 0.0f ? xs("Default") : xs("%.f%%");

						this->SliderFloat(xs("First Zoom"), &Config::Get<float>(Vars::World::FieldOfViewFirstZoom), 0.f, 1.f, fmt1, 100.f);
						this->SliderFloat(xs("Second Zoom"), &Config::Get<float>(Vars::World::FieldOfViewSecondZoom), 0.f, 1.f, fmt2, 100.f);
					});
				});

				this->DropdownEx(xs("Scope Overlay"), &Config::Get<int>(Vars::World::RemoveScope), { xs("Default"), xs("Lines"), xs("Gradient"), xs("Remove") }, [this]() {
					this->Checkbox(xs("Show Viewmodel In Scope"), &Config::Get<bool>(Vars::World::ShowViewmodelInScope));
				});

				Color_t* colsp[2]{ &Config::Get<Color_t>(Vars::World::PenetrationCrosshairColor1), &Config::Get<Color_t>(Vars::World::PenetrationCrosshairColor2) };
				this->CheckboxEx(xs("Penetration Crosshair"), &Config::Get<bool>(Vars::World::PenetrationCrosshair), false, colsp, 2, [this]() {
					this->Dropdown(xs("Type"), &Config::Get<int>(Vars::World::PenetrationCrosshairType), { xs("Dot"), xs("3D") });
				});

				this->ListEx(xs("Viewmodel"), [this]() {
					this->Checkbox(xs("Static"), &Config::Get<bool>(Vars::World::ViewmodelChangerStatic));
					this->SliderInt(xs("Field Of View"), &Config::Get<int>(Vars::World::ViewmodelChangerFov), 45, 120, xs("%d°"));

					this->CheckboxEx(xs("Position"), &Config::Get<bool>(Vars::World::ViewmodelChanger), false, nullptr, 0, [this]() {
						this->SliderFloat(xs("Position X"), &Config::Get<float>(Vars::World::ViewmodelChangerX), -10.f, 10.f, xs("%.f%%"), 10.f);
						this->SliderFloat(xs("Position Y"), &Config::Get<float>(Vars::World::ViewmodelChangerY), -10.f, 10.f, xs("%.f%%"), 10.f);
						this->SliderFloat(xs("Position Z"), &Config::Get<float>(Vars::World::ViewmodelChangerZ), -10.f, 10.f, xs("%.f%%"), 10.f);
					});

					this->CheckboxEx(xs("Rotation"), &Config::Get<bool>(Vars::World::ViewmodelChangerAngles), false, nullptr, 0, [this]() {
						this->SliderFloat(xs("Rotation X"), &Config::Get<float>(Vars::World::ViewmodelChangerPitch), -90.f, 90.f, xs("%.f°"));
						this->SliderFloat(xs("Rotation Y"), &Config::Get<float>(Vars::World::ViewmodelChangerYaw), -90.f, 90.f, xs("%.f°"));
						this->SliderFloat(xs("Rotation Z"), &Config::Get<float>(Vars::World::ViewmodelChangerRoll), -90.f, 90.f, xs("%.f°"));
					});
				});

				this->Dropdown(xs("Visual Recoil"), &Config::Get<int>(Vars::World::VisualRecoil), { xs("Default"), xs("Remove Shake"), xs("Remove All") });
				this->Dropdown(xs("Crosshair Settings"), &Config::Get<int>(Vars::World::Crosshair), { xs("Default"), xs("Disable"), xs("Force") });
				this->MultiDropdown(xs("Override Overhead Info"), &Config::Get<std::vector<int>>(Vars::World::Overhead), { xs("Remove Local"), xs("Remove Team") });
				this->MultiDropdown(xs("Removals"), &Config::Get<std::vector<int>>(Vars::World::RemovalsPerspective), { xs("Smoke"), xs("Flashbang"), xs("Torso"), xs("Team Intro"), xs("Decals") });
			}
			this->EndChild();
		}
		this->EndGroup();
	}
}