#include <shared.h>

void CGui::RenderMiscContent() {
	this->BeginGroup();
	{
		this->BeginChild(xs("Movement"));
		{
			this->Checkbox(xs("Bunny Hop"), &Config::Get<bool>(Vars::Movement::Bunnyhop));
			this->Checkbox(xs("Fast Stop"), &Config::Get<bool>(Vars::Movement::FastStop));
			this->Checkbox(xs("Fast Ladder"), &Config::Get<bool>(Vars::Movement::FastLadder));
			this->Checkbox(xs("Jump Bug"), &Config::Get<bool>(Vars::Movement::JumpBug));
			this->Checkbox(xs("Auto Strafer"), &Config::Get<bool>(Vars::Movement::Strafer));
			this->Checkbox(xs("Leg Movement"), &Config::Get<bool>(Vars::Movement::LegMovement));
		}
		this->EndChild();

		this->BeginChild(xs("Display"));
		{
			Color_t* watermark_color[1] = { &Config::Get<Color_t>(Vars::Misc::WatermarkAccentColor) };
			this->CheckboxEx(xs("Watermark"), &Config::Get<bool>(Vars::Misc::Watermark), false, watermark_color, 1, [this]() {
				this->Checkbox(xs("Show Username"), &Config::Get<bool>(Vars::Misc::WatermarkUsername));
				this->Checkbox(xs("Show Ping"), &Config::Get<bool>(Vars::Misc::WatermarkPing));
				this->Checkbox(xs("Show FPS"), &Config::Get<bool>(Vars::Misc::WatermarkFps));
				this->Checkbox(xs("Show Speed"), &Config::Get<bool>(Vars::Misc::WatermarkSpeed));
				this->Checkbox(xs("Show Time"), &Config::Get<bool>(Vars::Misc::WatermarkTime));
				this->SliderInt(xs("Watermark Opacity"), &Config::Get<int>(Vars::Misc::WatermarkOpacity), 0, 255);
			});

			this->CheckboxEx(xs("Indicators"), &Config::Get<bool>(Vars::Misc::Indicators), false, nullptr, 0, [this]() {
				this->SliderInt(xs("Indicators Opacity"), &Config::Get<int>(Vars::Misc::IndicatorOpacity), 0, 255);
			});

			if (Config::Get<bool>(Vars::Misc::Indicators)) {
				this->MultiDropdown(xs("Indicators List"), &Config::Get<std::vector<int>>(Vars::Misc::IndicatorItems), {
					xs("Doubletap"),
					xs("Hideshot"),
					xs("Bodyaim"),
					xs("Ping warning"),
					xs("Hitchance"),
					xs("Damage"),
					xs("Bomb"),
					xs("Peek assist")
				});
			}
		}
		this->EndChild();
	}
	this->EndGroup();

	this->Sameline();

	this->BeginGroup();
	{
		this->BeginChild(xs("Other"));
		{
			this->Checkbox(xs("Quick Switch"), &Config::Get<bool>(Vars::Misc::QuickSwitch));
			Color_t* hitmarker_color[1] = { &Config::Get<Color_t>(Vars::Misc::HitmarkerColor) };
			this->CheckboxEx(xs("Hitmarker"), &Config::Get<bool>(Vars::Misc::Hitmarker), false, hitmarker_color, 1);
			this->CheckboxEx(xs("Hit Sound"), &Config::Get<bool>(Vars::Misc::Hitsound), false, nullptr, 0, [this]() {
				this->SliderFloat(xs("Volume"), &Config::Get<float>(Vars::Misc::HitsoundVolume), 0.f, 1.f, xs("%.f%%"), 100.f);

				std::vector<std::string> items = { xs("Default"), xs("Coin"), xs("Press"), xs("Killcard"), xs("Level up"), xs("Gift"), xs("Star"), xs("Click"), xs("Headshot"), xs("Sparkle"), xs("Chocke"), xs("Call Of Duty"), xs("Tinka"), xs("Pig") };
				this->Dropdown(xs("On Body Hit"), &Config::Get<int>(Vars::Misc::HitsoundBodyHit), items);
				this->Dropdown(xs("On Body Kill"), &Config::Get<int>(Vars::Misc::HitsoundBodyKill), items);
				this->Dropdown(xs("On Head Hit"), &Config::Get<int>(Vars::Misc::HitsoundHeadHit), items);
				this->Dropdown(xs("On Head Kill"), &Config::Get<int>(Vars::Misc::HitsoundHeadKill), items);
			});

			if (this->Buttton(xs("Debug")))
				EngineClient->ExecuteClientCmd("sv_cheats 1; bot_stop 1; bot_add; sv_quantize_movement_input 0; sv_airaccelerate 300");
		}
		this->EndChild();

		this->BeginChild(xs("Buy Bot"));
		{
			this->Checkbox(xs("Enabled"), &Config::Get<bool>(Vars::Misc::BuyBot));
			this->Dropdown(xs("Primary"), &Config::Get<int>(Vars::Misc::BuyBotPrimary), { xs("None"), xs("Auto Sniper"), xs("SSG 08"), xs("AWP"), xs("AK-47 / M4A4"), xs("AK-47 / M4A1-S") });
			this->Dropdown(xs("Secondary"), &Config::Get<int>(Vars::Misc::BuyBotSecondary), { xs("None"), xs("Desert Eagle"), xs("R8 Revolver"), xs("Dual Berettas"), xs("P250"), xs("Tec-9 / Five-SeveN") });
			this->Checkbox(xs("Armor"), &Config::Get<bool>(Vars::Misc::BuyBotArmor));
			this->Checkbox(xs("Defuse Kit"), &Config::Get<bool>(Vars::Misc::BuyBotDefuser));
			this->MultiDropdown(xs("Grenades"), &Config::Get<std::vector<int>>(Vars::Misc::BuyBotGrenades), { xs("HE Grenade"), xs("Smoke"), xs("Molotov / Incendiary"), xs("Flashbang") });
		}
		this->EndChild();
	}
	this->EndGroup();
}