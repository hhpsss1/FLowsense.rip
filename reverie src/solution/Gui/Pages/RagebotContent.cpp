#include <shared.h>

void CGui::RenderRagebotContent() {
	static int weapon_group = 0;
	const std::vector<std::string> hitboxes = { xs("Head"), xs("Chest"), xs("Stomach"), xs("Arms"), xs("Legs"), xs("Feet") };

#define WPN_CFG(type, var) (weapon_group == 1 ? &Config::Get<type>(Vars::Ragebot::Awp::var) : (weapon_group == 2 ? &Config::Get<type>(Vars::Ragebot::AutoSniper::var) : (weapon_group == 3 ? &Config::Get<type>(Vars::Ragebot::Scout::var) : (weapon_group == 4 ? &Config::Get<type>(Vars::Ragebot::Deagle::var) : (weapon_group == 5 ? &Config::Get<type>(Vars::Ragebot::Revolver::var) : (weapon_group == 6 ? &Config::Get<type>(Vars::Ragebot::Pistol::var) : &Config::Get<type>(Vars::Ragebot::Global::var)))))))

	this->BeginGroup();
	{
		this->BeginChild(xs("Main"));
		{
			this->CheckboxEx(xs("Enabled"), &Config::Get<bool>(Vars::Ragebot::Enable), false, nullptr, 0, [this]() {
				this->Dropdown(xs("Weapon"), &weapon_group, { xs("Global"), xs("AWP"), xs("Auto Sniper"), xs("Scout"), xs("Deagle"), xs("Revolver"), xs("Pistol") });
				this->Checkbox(xs("Zeusbot"), &Config::Get<bool>(Vars::Ragebot::Zeusbot));
				this->Checkbox(xs("Knifebot"), &Config::Get<bool>(Vars::Ragebot::Knifebot));
				this->CheckboxEx(xs("Show Aimpoints"), &Config::Get<bool>(Vars::Ragebot::ShowAimpoints), false, nullptr, 0, [this]() {
					this->SliderInt(xs("Scan Per Ticks"), &Config::Get<int>(Vars::Ragebot::TicksPerAimpoint), 1, 8, xs("%d ticks"));
				});
			});

			this->Checkbox(xs("Silent Aim"), WPN_CFG(bool, SilentAim));
			this->Checkbox(xs("Automatic Fire"), WPN_CFG(bool, AutomaticFire));
			this->Checkbox(xs("Aim Through Walls"), WPN_CFG(bool, PenetrateWalls));
			this->Checkbox(xs("Double Precision"), &Config::Get<bool>(Vars::Ragebot::DoublePrecision));
			this->Checkbox(xs("Prepare Revolver"), &Config::Get<bool>(Vars::Ragebot::PrepareRevolver));
			this->SliderFloat(xs("Field of View"), WPN_CFG(float, MaximumFov), 0.f, 180.f, xs("%.1f°"));
		}
		this->EndChild();

		this->BeginChild(xs("Selection"));
		{
			this->Dropdown(xs("Target"), WPN_CFG(int, TargetSelection), { xs("Highest Damage"), xs("Distance"), xs("Field of View"), xs("Lowest Health") });
			this->MultiDropdown(xs("Hitboxes"), WPN_CFG(std::vector<bool>, Hitboxes), hitboxes, [this]() { 
				this->Checkbox(xs("Force Body Aim"), WPN_CFG(bool, ForceBodyAim));  
			});
			this->MultiDropdown(xs("Multipoint"), WPN_CFG(std::vector<bool>, Multipoints), hitboxes, [this]() {
				this->SliderFloat(xs("Head Scale"), WPN_CFG(float, PointScale), 0.f, 100.f, xs("%.0f%%"));
				this->SliderFloat(xs("Body Scale"), WPN_CFG(float, BodyPointScale), 0.f, 100.f, xs("%.0f%%"));
			});

			this->SliderFloatEx(xs("Hit Chance"), WPN_CFG(float, Hitchance), 0.f, 100.f, [this]() {
				this->Checkbox(xs("Strict Hitchance"), WPN_CFG(bool, StrictHitchance));
				this->Checkbox(xs("Automatic Scope"), WPN_CFG(bool, AutomaticScope));
			}, xs("%.0f%%"));

			this->MultiDropdown(xs("Auto Stop"), WPN_CFG(std::vector<bool>, AutoStop), { xs("Early"), xs("Stop Between Shots"), xs("In Air"), xs("On Land") });

			const char* lethal = (*WPN_CFG(int, MinimumDamage) == 100) ? xs("Lethal") : xs("%d");
			this->SliderInt(xs("Minimum Damage"), WPN_CFG(int, MinimumDamage), 1, 200, lethal);
			
			const char* distance_fmt = (*WPN_CFG(float, MaximumDistance) == 0.f) ? xs("Infinite") : xs("%.f Units");
			this->SliderFloat(xs("Maximum Distance"), WPN_CFG(float, MaximumDistance), 0.f, 4096.f, distance_fmt);
		}
		this->EndChild();
	}
	this->EndGroup();

	this->Sameline();

	this->BeginGroup();
	{
		this->BeginChild(xs("Other"));
		{
			this->CheckboxEx(xs("Extrapolation"), &Config::Get<bool>(Vars::Ragebot::Extrapolation), false, nullptr, 0, [this]() {
				this->SliderInt(xs("Maximum Ticks"), &Config::Get<int>(Vars::Ragebot::ExtrapolationTicks), 1, 8);
			});
			this->CheckboxEx(xs("Delay Shot"), WPN_CFG(bool, DelayShotEnable), false, nullptr, 0, [this]() { 
				this->SliderInt(xs("Ticks"), WPN_CFG(int, DelayShot), 1, 16); 
			});
			this->Checkbox(xs("Remove Recoil"), WPN_CFG(bool, RemoveRecoil));
			this->Checkbox(xs("Duck Peek Assist"), &Config::Get<bool>(Vars::Ragebot::DuckPeek));

			const char* pa_fmt = (Config::Get<float>(Vars::Movement::PeekAssistMaxDistance) == 0.f) ? xs("Infinite") : xs("%.f units");

			Color_t* pac[1] = { &Config::Get<Color_t>(Vars::Movement::PeekAssistColor) };
			this->CheckboxEx(xs("Quick Peek Assist"), &Config::Get<bool>(Vars::Movement::PeekAssist), false, pac, 1, [this, pa_fmt]() {
				this->MultiDropdown(xs("Modes"), &Config::Get<std::vector<int>>(Vars::Movement::PeekAssistTypes), { xs("On Key Release"), xs("On Shot"), xs("On Grenade Throw"), xs("On Jump") });
				this->SliderFloat(xs("Max Distance"), &Config::Get<float>(Vars::Movement::PeekAssistMaxDistance), 0.f, 300.f, pa_fmt);
				this->Checkbox(xs("Jump On Retreat"), &Config::Get<bool>(Vars::Movement::PeekAssistJumpOnRetreat));
			});
		}
		this->EndChild();

		this->BeginChild(xs("Anti-Aim"));
		{
			this->Checkbox(xs("Enable"), &Config::Get<bool>(Vars::Antiaim::Enable));
			this->Checkbox(xs("At Targets"), &Config::Get<bool>(Vars::Antiaim::AtTargets));

			Color_t* clrs[2] = { &Config::Get<Color_t>(Vars::Antiaim::MouseOverrideColor1), &Config::Get<Color_t>(Vars::Antiaim::MouseOverrideColor2) };
			this->CheckboxEx(xs("Mouse Override"), &Config::Get<bool>(Vars::Antiaim::MouseOverride), false, clrs, 2, [this]() { 
				this->SliderFloat(xs("Sensitivity"), &Config::Get<float>(Vars::Antiaim::MouseOverrideSensitivity), 1.f, 200.f, xs("%.f%%")); 
			});

			this->Dropdown(xs("Pitch"), &Config::Get<int>(Vars::Antiaim::Pitch), { xs("None"), xs("Down"), xs("Up") });

			this->DropdownEx(xs("Yaw"), &Config::Get<int>(Vars::Antiaim::Yaw), { xs("Forward"), xs("Backward") }, [this]() {
				this->Checkbox(xs("Auto Correction"), &Config::Get<bool>(Vars::Antiaim::YawCorrection));
				this->SliderInt(xs("Offset"), &Config::Get<int>(Vars::Antiaim::YawOffset), -90, 90, xs("%d°"));
			});

			this->CheckboxEx(xs("Spinbot"), &Config::Get<bool>(Vars::Antiaim::Spinbot), false, nullptr, 0, [this]() { 
				this->SliderInt(xs("Speed"), &Config::Get<int>(Vars::Antiaim::SpinbotValue), 0, 180, xs("%d°")); 
			});

			Color_t* indicator_clr[1] = { &Config::Get<Color_t>(Vars::Antiaim::DirectionIndicatorColor) };
			this->CheckboxEx(xs("Direction Indicator"), &Config::Get<bool>(Vars::Antiaim::DirectionIndicator), false, indicator_clr, 1);
		}
		this->EndChild();
	}
	this->EndGroup();

#undef WPN_CFG
}
