#pragma once

#include <any>
#include <string>
#include <vector>
#include <filesystem>
#include <sstream>
#include <unordered_map>
#include <xorstring/xorstring.h>

#define Serialize(type, name, def) inline const std::uint32_t name = Config::AddVariable<type>(xs(#name), xs(#type), def)
#define SerializeNamed(type, name, savename, def) inline const std::uint32_t name = Config::AddVariable<type>(xs(savename), xs(#type), def)
#define SerializeVector(type, name, def) inline const std::uint32_t name = Config::AddVariable<std::vector<type>>(xs(#name), xs("std::vector<" #type ">"), def)
#define SerializeGroup(grp, type, name, def) inline const std::uint32_t name = Config::AddVariable<type>(xs(#grp "_" #name), xs(#type), def)
#define SerializeGroupVector(grp, type, name, def) inline const std::uint32_t name = Config::AddVariable<std::vector<type>>(xs(#grp "_" #name), xs("std::vector<" #type ">"), def)

struct VariableObject_t {
    std::string name;
    std::string type_name;
    std::any value;
    std::any default_value;

    template <typename t>
    t& Get() { return *std::any_cast<t>(&value); }

    template <typename t>
    void Set(const t& val) { value.emplace<t>(val); }

    template <typename t>
    t GetDefault() const { return std::any_cast<t>(default_value); }
};

struct ConfigItem_t {
    std::string name;
    std::uint64_t created_at{ 0 };
    std::string created_text;
    bool is_default{ false };
};

namespace Config {
    inline std::filesystem::path base_path{};
    inline std::vector<VariableObject_t> variables{};
    inline std::vector<ConfigItem_t> items{};
    inline std::string active_config{};
    inline std::string startup_config{ xs("default") };

    template <typename t>
    inline std::uint32_t AddVariable(const std::string& name, const std::string& type_name, const t& def) {
        variables.push_back({ name, type_name, std::make_any<t>(def), std::make_any<t>(def) });
        return static_cast<std::uint32_t>(variables.size() - 1u);
    }

    template <typename t>
    inline t& Get(const std::uint32_t idx) { return variables.at(idx).Get<t>(); }

    template <typename t>
    inline void Set(const std::uint32_t idx, const t& val) { variables.at(idx).Set<t>(val); }

    inline const std::vector<ConfigItem_t>& GetItems() { return items; }
    inline const std::string& GetActiveConfig() { return active_config; }
    inline const std::string& GetStartupConfig() { return startup_config; }
    inline bool IsActive(const std::string& name) { return active_config == name; }
    inline bool IsStartup(const std::string& name) { return startup_config == name; }

    std::filesystem::path GetConfigPath(const std::string&);
    std::filesystem::path GetAutoloadPath();

    std::string SanitizeName(std::string);
    std::string FormatDate(std::uint64_t);

    bool IsDefaultConfig(const std::string&);
    bool Exists(const std::string&);

    int GetIndexFromPtr(const void* ptr);
    void* GetPtrFromName(const std::string& name, const std::string& type_name);
    void* GetLegacyBindPtr(int idx, const std::string& type_name);

    void Init();
    void Refresh();
    void LoadStartup();

    bool Save(const std::string&);
    bool Load(const std::string&);
    bool Create(const std::string&);
    bool Remove(const std::string&);
    bool SetStartupConfig(const std::string&);
    bool ShowInExplorer(const std::string&);
}

#define DEFINE_RAGE_WEAPON_GROUP(group_name) \
    namespace group_name { \
        SerializeGroup(group_name, bool, AutomaticFire, true); \
        SerializeGroup(group_name, bool, AutomaticScope, true); \
        SerializeGroupVector(group_name, bool, AutoStop, (std::vector<bool>{ true, false, false, false })); \
        SerializeGroup(group_name, bool, PenetrateWalls, true); \
        SerializeGroup(group_name, bool, SilentAim, true); \
        SerializeGroup(group_name, bool, Zeusbot, false); \
        SerializeGroup(group_name, bool, Knifebot, false); \
        SerializeGroup(group_name, float, Hitchance, 75.f); \
        SerializeGroup(group_name, int, MinimumDamage, 100); \
        SerializeGroup(group_name, float, PointScale, 70.f); \
        SerializeGroup(group_name, float, MaximumFov, 180.f); \
        SerializeGroup(group_name, float, MaximumDistance, 0.f); \
        SerializeGroup(group_name, float, StopSpeed, 50.f); \
        SerializeGroup(group_name, int, TicksPerAimpoint, 4); \
        SerializeGroup(group_name, float, BodyPointScale, 70.f); \
        SerializeGroup(group_name, int, TargetSelection, 0); \
        /* No backtrack: History / ExtendedBacktrack removed. */ \
        SerializeGroup(group_name, bool, DelayShotEnable, false); \
        SerializeGroup(group_name, int, DelayShot, 1); \
        SerializeGroup(group_name, bool, RemoveRecoil, true); \
        SerializeGroup(group_name, bool, StrictHitchance, true); \
        SerializeGroup(group_name, bool, ForceBodyAim, false); \
        SerializeGroupVector(group_name, bool, Hitboxes, (std::vector<bool>{ true, true, true, false, false, false })); \
        SerializeGroupVector(group_name, bool, Multipoints, std::vector<bool>(6, true)); \
        SerializeGroup(group_name, bool, HitboxHead, true); \
        SerializeGroup(group_name, bool, HitboxChest, true); \
        SerializeGroup(group_name, bool, HitboxStomach, true); \
        SerializeGroup(group_name, bool, HitboxArms, false); \
        SerializeGroup(group_name, bool, HitboxLegs, false); \
        SerializeGroup(group_name, bool, HitboxFeet, false); \
        SerializeGroup(group_name, bool, MultipointHead, true); \
        SerializeGroup(group_name, bool, MultipointChest, true); \
        SerializeGroup(group_name, bool, MultipointStomach, true); \
        SerializeGroup(group_name, bool, MultipointArms, true); \
        SerializeGroup(group_name, bool, MultipointLegs, true); \
        SerializeGroup(group_name, bool, MultipointFeet, true); \
        /* --- CS2 hitbox resolver --- */ \
        SerializeGroup(group_name, bool, Resolver, true); \
        SerializeGroup(group_name, int, ResolverMode, 0); \
        SerializeGroup(group_name, int, ResolverHitboxPriority, 0); \
        SerializeGroup(group_name, float, ResolverYawRange, 58.f); \
    }

#define DEFINE_PLAYER_ESP_GROUP(group_name) \
    namespace group_name { \
        SerializeGroup(group_name, bool, Enable, false); \
        SerializeGroup(group_name, bool, Name, false); \
        SerializeGroup(group_name, Color_t, NameColor, Color_t(255, 255, 255, 200)); \
        SerializeGroup(group_name, int, NameFont, 0); \
        SerializeGroup(group_name, bool, Box, false); \
        SerializeGroup(group_name, bool, BoxGradient, false); \
        SerializeGroup(group_name, bool, BoxGlow, false); \
        SerializeGroup(group_name, Color_t, BoxColor, Color_t(255, 255, 255, 200)); \
        SerializeGroup(group_name, Color_t, BoxGradientColor, Color_t(255, 255, 255, 200)); \
        SerializeGroup(group_name, bool, HealthBar, false); \
        SerializeGroup(group_name, bool, HealthBarGradient, false); \
        SerializeGroup(group_name, bool, HealthBarGlow, false); \
        SerializeGroup(group_name, Color_t, HealthBarColor, Color_t(255, 255, 255, 200)); \
        SerializeGroup(group_name, Color_t, HealthBarGradientColor, Color_t(255, 255, 255, 200)); \
        SerializeGroup(group_name, int, HealthBarText, 1); \
        SerializeGroup(group_name, int, HealthBarFont, 2); \
        SerializeGroup(group_name, bool, AmmoBar, false); \
        SerializeGroup(group_name, bool, AmmoBarGradient, false); \
        SerializeGroup(group_name, bool, AmmoBarGlow, false); \
        SerializeGroup(group_name, Color_t, AmmoBarColor, Color_t(80, 170, 255, 200)); \
        SerializeGroup(group_name, Color_t, AmmoBarGradientColor, Color_t(80, 170, 255, 200)); \
        SerializeGroup(group_name, int, AmmoBarText, 0); \
        SerializeGroup(group_name, int, AmmoBarFont, 2); \
        SerializeGroup(group_name, int, LayoutABarSide, 2); \
        SerializeGroup(group_name, int, LayoutABarAlign, 1); \
        SerializeGroup(group_name, int, LayoutABarOrder, 0); \
        SerializeGroup(group_name, bool, ArmorBar, false); \
        SerializeGroup(group_name, bool, ArmorBarGradient, false); \
        SerializeGroup(group_name, bool, ArmorBarGlow, false); \
        SerializeGroup(group_name, Color_t, ArmorBarColor, Color_t(220, 220, 220, 200)); \
        SerializeGroup(group_name, Color_t, ArmorBarGradientColor, Color_t(220, 220, 220, 200)); \
        SerializeGroup(group_name, int, ArmorBarText, 0); \
        SerializeGroup(group_name, int, ArmorBarFont, 2); \
        SerializeGroup(group_name, int, LayoutArBarSide, 3); \
        SerializeGroup(group_name, int, LayoutArBarAlign, 1); \
        SerializeGroup(group_name, int, LayoutArBarOrder, 1); \
        SerializeGroup(group_name, bool, WeaponName, false); \
        SerializeGroup(group_name, Color_t, WeaponNameColor, Color_t(255, 255, 255, 200)); \
        SerializeGroup(group_name, int, WeaponNameFont, 0); \
        SerializeGroup(group_name, bool, WeaponIcon, false); \
        SerializeGroup(group_name, Color_t, WeaponIconColor, Color_t(255, 255, 255, 200)); \
        SerializeGroup(group_name, bool, GrenadeIcons, false); \
        SerializeGroup(group_name, Color_t, GrenadeIconsColor, Color_t(255, 255, 255, 200)); \
        SerializeGroup(group_name, bool, Skeleton, false); \
        SerializeGroup(group_name, Color_t, SkeletonColor, Color_t(255, 255, 255, 200)); \
        SerializeGroup(group_name, bool, Fading, false); \
        SerializeGroup(group_name, float, FadingDuration, 0.5f); \
        SerializeGroup(group_name, int, LayoutNameSide, 0); \
        SerializeGroup(group_name, int, LayoutNameAlign, 1); \
        SerializeGroup(group_name, int, LayoutNameOrder, 0); \
        SerializeGroup(group_name, int, LayoutHBarSide, 3); \
        SerializeGroup(group_name, int, LayoutHBarAlign, 1); \
        SerializeGroup(group_name, int, LayoutHBarOrder, 0); \
        SerializeGroup(group_name, int, LayoutWNameSide, 2); \
        SerializeGroup(group_name, int, LayoutWNameAlign, 1); \
        SerializeGroup(group_name, int, LayoutWNameOrder, 0); \
        SerializeGroup(group_name, int, LayoutWIconSide, 2); \
        SerializeGroup(group_name, int, LayoutWIconAlign, 1); \
        SerializeGroup(group_name, int, LayoutWIconOrder, 1); \
        SerializeGroup(group_name, int, LayoutGrenadeIconsSide, 2); \
        SerializeGroup(group_name, int, LayoutGrenadeIconsAlign, 1); \
        SerializeGroup(group_name, int, LayoutGrenadeIconsOrder, 2); \
        SerializeGroup(group_name, bool, FlagsScoped, false); \
        SerializeGroup(group_name, Color_t, FlagsScopedColor, Color_t(120, 200, 255, 200)); \
        SerializeGroup(group_name, int, LayoutFlagScopedSide, 1); \
        SerializeGroup(group_name, int, LayoutFlagScopedAlign, 0); \
        SerializeGroup(group_name, int, LayoutFlagScopedOrder, 1); \
        SerializeGroup(group_name, int, LayoutFlagScopedFont, 2); \
        SerializeGroup(group_name, bool, FlagsBomb, false); \
        SerializeGroup(group_name, Color_t, FlagsBombColor, Color_t(255, 80, 80, 200)); \
        SerializeGroup(group_name, int, LayoutFlagBombSide, 1); \
        SerializeGroup(group_name, int, LayoutFlagBombAlign, 0); \
        SerializeGroup(group_name, int, LayoutFlagBombOrder, 2); \
        SerializeGroup(group_name, int, LayoutFlagBombFont, 2); \
        SerializeGroup(group_name, bool, FlagsTaser, false); \
        SerializeGroup(group_name, Color_t, FlagsTaserColor, Color_t(255, 220, 80, 200)); \
        SerializeGroup(group_name, int, LayoutFlagTaserSide, 1); \
        SerializeGroup(group_name, int, LayoutFlagTaserAlign, 0); \
        SerializeGroup(group_name, int, LayoutFlagTaserOrder, 3); \
        SerializeGroup(group_name, int, LayoutFlagTaserFont, 2); \
        SerializeGroup(group_name, bool, FlagsDefuser, false); \
        SerializeGroup(group_name, Color_t, FlagsDefuserColor, Color_t(80, 255, 140, 200)); \
        SerializeGroup(group_name, int, LayoutFlagDefuserSide, 1); \
        SerializeGroup(group_name, int, LayoutFlagDefuserAlign, 0); \
        SerializeGroup(group_name, int, LayoutFlagDefuserOrder, 4); \
        SerializeGroup(group_name, int, LayoutFlagDefuserFont, 2); \
        SerializeGroup(group_name, bool, FlagsPin, false); \
        SerializeGroup(group_name, Color_t, FlagsPinColor, Color_t(255, 140, 60, 200)); \
        SerializeGroup(group_name, int, LayoutFlagPinSide, 1); \
        SerializeGroup(group_name, int, LayoutFlagPinAlign, 0); \
        SerializeGroup(group_name, int, LayoutFlagPinOrder, 5); \
        SerializeGroup(group_name, int, LayoutFlagPinFont, 2); \
        SerializeGroup(group_name, bool, FlagsMoney, false); \
        SerializeGroup(group_name, Color_t, FlagsMoneyColor, Color_t(120, 255, 120, 200)); \
        SerializeGroup(group_name, int, LayoutFlagMoneySide, 1); \
        SerializeGroup(group_name, int, LayoutFlagMoneyAlign, 0); \
        SerializeGroup(group_name, int, LayoutFlagMoneyOrder, 6); \
        SerializeGroup(group_name, int, LayoutFlagMoneyFont, 2); \
        SerializeGroup(group_name, bool, FlagsReload, false); \
        SerializeGroup(group_name, Color_t, FlagsReloadColor, Color_t(255, 255, 255, 200)); \
        SerializeGroup(group_name, int, LayoutFlagReloadSide, 1); \
        SerializeGroup(group_name, int, LayoutFlagReloadAlign, 0); \
        SerializeGroup(group_name, int, LayoutFlagReloadOrder, 7); \
        SerializeGroup(group_name, int, LayoutFlagReloadFont, 2); \
        SerializeGroup(group_name, bool, FlagsPing, false); \
        SerializeGroup(group_name, Color_t, FlagsPingColor, Color_t(200, 200, 200, 200)); \
        SerializeGroup(group_name, int, LayoutFlagPingSide, 1); \
        SerializeGroup(group_name, int, LayoutFlagPingAlign, 0); \
        SerializeGroup(group_name, int, LayoutFlagPingOrder, 8); \
        SerializeGroup(group_name, int, LayoutFlagPingFont, 2); \
        SerializeGroup(group_name, bool, FlagsArmor, false); \
        SerializeGroup(group_name, Color_t, FlagsArmorColor, Color_t(160, 190, 255, 200)); \
        SerializeGroup(group_name, int, LayoutFlagArmorSide, 1); \
        SerializeGroup(group_name, int, LayoutFlagArmorAlign, 0); \
        SerializeGroup(group_name, int, LayoutFlagArmorOrder, 9); \
        SerializeGroup(group_name, int, LayoutFlagArmorFont, 2); \
        SerializeGroup(group_name, bool, FlagsSlow, false); \
        SerializeGroup(group_name, Color_t, FlagsSlowColorFull, Color_t(255, 60, 60, 200)); \
        SerializeGroup(group_name, Color_t, FlagsSlowColorNormal, Color_t(255, 220, 120, 200)); \
        SerializeGroup(group_name, int, LayoutFlagSlowSide, 1); \
        SerializeGroup(group_name, int, LayoutFlagSlowAlign, 0); \
        SerializeGroup(group_name, int, LayoutFlagSlowOrder, 10); \
        SerializeGroup(group_name, int, LayoutFlagSlowFont, 2); \
    }

#define DEFINE_CHAMS_GROUP(group_name, enable_name) \
    namespace group_name { \
        SerializeNamed(bool, Enable, enable_name, false); \
        SerializeGroup(group_name, Color_t, Color, Color_t::White()); \
        SerializeGroup(group_name, bool, Overlay, false); \
        SerializeGroup(group_name, int, Material, 0); \
        SerializeGroup(group_name, float, Exponent, 2.0f); \
        SerializeGroup(group_name, float, Falloff, 10.0f); \
    }

enum : uint32_t {
    overhead_remove_local = 0,
    overhead_remove_team,
    overhead_max
};

enum : uint32_t {
    remove_smoke,
    remove_flash,
    remove_torso,
    remove_team_intro,
    remove_decals,
    remove_max
};

enum : uint32_t {
    peek_assist_types_on_key_release = 0,
    peek_assist_types_on_shot,
    peek_assist_types_on_grenade_throw,
    peek_assist_types_on_jump,
    peek_assist_types_max
};

namespace Vars {
    namespace Binds {
        Serialize(std::string, Data, "");
    }

    namespace Ragebot {
        SerializeNamed(bool, Enable, "Ragebot_Enable", false);
        Serialize(bool, ForceShot, false);
        Serialize(bool, Zeusbot, false);
        Serialize(bool, Knifebot, false);
        Serialize(bool, DuckPeek, false);
        Serialize(bool, ShowAimpoints, false);
        Serialize(bool, DoublePrecision, false);
        Serialize(bool, PrepareRevolver, false);
        Serialize(bool, Extrapolation, false);
        Serialize(int, ExtrapolationTicks, 8);
        Serialize(bool, Prediction, true);
        Serialize(bool, Backtrack, false);
        Serialize(int, BacktrackTicks, 12);
        Serialize(bool, BacktrackPrediction, true);
        Serialize(int, BacktrackPredictionTicks, 6);
        Serialize(int, TicksPerAimpoint, 4);
        // --- дополнительные из их конфига ---
        Serialize(bool, DoubleTap, false);
        Serialize(float, KillDelay, 0.f);
        Serialize(float, AimSpeed, 0.f);
        Serialize(float, ReactionTime, 0.f);
        Serialize(int, MaxWalls, 2);
        Serialize(bool, SelfTrack, false);
        Serialize(int, HitboxMask, 0);
        Serialize(int, RemoveSpreadMode, 0);
        Serialize(int, FutureTicks, 0);
        Serialize(int, DelayShotDamage, 0);
        Serialize(int, History, 12);

        DEFINE_RAGE_WEAPON_GROUP(Global);
        DEFINE_RAGE_WEAPON_GROUP(AutoSniper);
        DEFINE_RAGE_WEAPON_GROUP(Scout);
        DEFINE_RAGE_WEAPON_GROUP(Awp);
        DEFINE_RAGE_WEAPON_GROUP(Deagle);
        DEFINE_RAGE_WEAPON_GROUP(Revolver);
        DEFINE_RAGE_WEAPON_GROUP(Pistol);
    }

    namespace Antiaim {
        SerializeNamed(bool, Enable, "Antiaim_Enable", false);
        Serialize(bool, AtTargets, false);
        Serialize(bool, MouseOverride, false);
        Serialize(float, MouseOverrideSensitivity, 100.f);
        Serialize(Color_t, MouseOverrideColor1, Color_t::Blue());
        Serialize(Color_t, MouseOverrideColor2, Color_t::Red());
        Serialize(bool, DirectionIndicator, false);
        Serialize(Color_t, DirectionIndicatorColor, Color_t(255, 255, 255, 220));
        Serialize(bool, Spinbot, false);
        Serialize(int, SpinbotValue, 0);
        Serialize(int, Pitch, 0);
        Serialize(int, Yaw, 0);
        Serialize(int, YawOffset, 0);
        Serialize(bool, YawCorrection, false);
    }

    namespace World {
        Serialize(bool, AspectRatio, false);
        Serialize(float, AspectRatioValue, 0.0f);
        Serialize(bool, FieldOfViewEnabled, false);
        Serialize(float, FieldOfView, 90.0f);
        Serialize(float, FieldOfViewFirstZoom, 0.0f);
        Serialize(float, FieldOfViewSecondZoom, 0.0f);
        Serialize(bool, Thirdperson, false);
        Serialize(bool, ThirdpersonDisableAnimation, false);
        Serialize(int, ThirdpersonDistance, 50);
        Serialize(int, RemoveScope, 0);
        Serialize(bool, ShowViewmodelInScope, false);
        SerializeNamed(bool, Enable, "World_Enable", false);
        Serialize(bool, ModulateWorld, false);
        Serialize(Color_t, WorldColor, Color_t::White());
        Serialize(bool, ModulateLight, false);
        Serialize(Color_t, LightColor, Color_t::White());
        Serialize(bool, ModulateSkybox, false);
        Serialize(Color_t, SkyboxColor, Color_t::White());
        Serialize(bool, ModulateSun, false);
        Serialize(Color_t, SunColor, Color_t::White());
        Serialize(bool, ModulateClouds, false);
        Serialize(Color_t, CloudsColor, Color_t::White());
        Serialize(bool, ModelWetness, false);
        Serialize(float, ModelWetnessCoverage, 4.f);
        Serialize(float, ModelWetnessSpeed, 1.f);
        Serialize(bool, Bloom, false);
        Serialize(float, BloomIntensity, 0.f);
        Serialize(bool, Gamma, false);
        Serialize(float, GammaValue, 2.2f);
        Serialize(bool, DepthOfField, false);
        Serialize(float, DepthOfFieldNearDistance, 0.0f);
        Serialize(float, DepthOfFieldFarDistance, 2000.0f);
        Serialize(bool, Fullbright, false);
        Serialize(Color_t, FullbrightColor, Color_t::White());
        Serialize(float, Exposure, 100.0f);
        SerializeVector(int, Removals, std::vector<int>(4, 0));
        SerializeVector(int, RemovalsPerspective, std::vector<int>(remove_max, 0));
        SerializeVector(int, Overhead, std::vector<int>(overhead_max, 0));
        Serialize(int, VisualRecoil, 0);
        Serialize(int, Crosshair, 0);
        Serialize(bool, PenetrationCrosshair, false);
        Serialize(int, PenetrationCrosshairType, 0);
        Serialize(Color_t, PenetrationCrosshairColor1, Color_t::Green());
        Serialize(Color_t, PenetrationCrosshairColor2, Color_t::Red());
        Serialize(bool, SkyboxChanger, false);
        Serialize(int, SkyboxChangerType, 0);
        Serialize(bool, ViewmodelChanger, false);
        Serialize(float, ViewmodelChangerX, 0.f);
        Serialize(float, ViewmodelChangerY, 0.f);
        Serialize(float, ViewmodelChangerZ, 0.f);
        Serialize(bool, ViewmodelChangerAngles, false);
        Serialize(float, ViewmodelChangerPitch, 0.f);
        Serialize(float, ViewmodelChangerYaw, 0.f);
        Serialize(float, ViewmodelChangerRoll, 0.f);
        Serialize(bool, ViewmodelChangerStatic, false);
        Serialize(int, ViewmodelChangerFov, 68);
    }

    namespace Player {
        DEFINE_PLAYER_ESP_GROUP(Enemy);
        DEFINE_PLAYER_ESP_GROUP(Friendly);
        DEFINE_PLAYER_ESP_GROUP(Local);
    }

    namespace ColoredModels {
        DEFINE_CHAMS_GROUP(EnemyVisible, "EnemyVisible_Enable");
        DEFINE_CHAMS_GROUP(EnemyOccluded, "EnemyOccluded_Enable");
        DEFINE_CHAMS_GROUP(FriendlyVisible, "FriendlyVisible_Enable");
        DEFINE_CHAMS_GROUP(FriendlyOccluded, "FriendlyOccluded_Enable");
        DEFINE_CHAMS_GROUP(Local, "ColoredModels_Local_Enable");
        DEFINE_CHAMS_GROUP(Arms, "Arms_Enable");
        DEFINE_CHAMS_GROUP(Weapon, "Weapon_Enable");
        DEFINE_CHAMS_GROUP(Attachments, "Attachments_Enable");

        Serialize(bool, ScopeBlend, false);
        Serialize(int, ScopeBlendValue, 100);
        Serialize(bool, ScopeBlendKeepOnResume, false);
    }

    namespace Effects {
        Serialize(bool, BeamsLocal, false);
        Serialize(Color_t, BeamsLocalColor, Color_t::Blue());
        Serialize(int, BeamsLocalDuration, 4);
        Serialize(float, BeamsLocalWidth, 2.f);

        Serialize(bool, BeamsEnemy, false);
        Serialize(Color_t, BeamsEnemyColor, Color_t::Red());
        Serialize(int, BeamsEnemyDuration, 4);
        Serialize(float, BeamsEnemyWidth, 2.f);

        Serialize(bool, BeamsFriendly, false);
        Serialize(Color_t, BeamsFriendlyColor, Color_t::Green());
        Serialize(int, BeamsFriendlyDuration, 4);
        Serialize(float, BeamsFriendlyWidth, 2.f);

        Serialize(bool, ImpactsServer, false);
        Serialize(float, ImpactsServerSize, 2.f);
        Serialize(float, ImpactsServerDuration, 4.f);
        Serialize(Color_t, ImpactsServerColor, Color_t::Blue());

        Serialize(bool, ImpactsClient, false);
        Serialize(float, ImpactsClientSize, 2.f);
        Serialize(float, ImpactsClientDuration, 4.f);
        Serialize(Color_t, ImpactsClientColor, Color_t::Red());

        Serialize(bool, Soul, false);
        Serialize(bool, SoulZ, false);
        Serialize(Color_t, SoulColor, Color_t::White());
        Serialize(float, SoulDuration, 4.f);
        Serialize(float, SoulDensity, 500.f);
        Serialize(float, SoulIntensity, 1.f);
        Serialize(float, SoulGlowIntensity, 1.f);
        Serialize(float, SoulGravityX, 0.f);
        Serialize(float, SoulGravityY, 0.f);
        Serialize(float, SoulGravityZ, 0.f);
        Serialize(float, SoulWind, 0.f);

        Serialize(bool, Weather, false);
        Serialize(int, WeatherType, 0);

        Serialize(float, StarsIntensity, 2.f);
        Serialize(float, StarsGlowIntensity, 2.f);
        Serialize(float, StarsDensity, 500.f);
        Serialize(Color_t, StarsColor, Color_t::White());
        Serialize(float, StarsGravityX, 0.f);
        Serialize(float, StarsGravityY, 0.f);
        Serialize(float, StarsGravityZ, 0.f);

        Serialize(float, SnowDensity, 500.f);
        Serialize(Color_t, SnowColor, Color_t::White());
        Serialize(float, SnowSize, 0.1f);
        Serialize(float, SnowGlowIntensity, 0.3f);
        Serialize(float, SnowGravityX, 0.f);
        Serialize(float, SnowGravityY, 0.f);
        Serialize(float, SnowGravityZ, -1000.f);
        Serialize(float, SnowWind, 0.f);
        Serialize(float, SnowOpacity, 1.f);
        Serialize(float, SnowGlowOpacity, 1.f);

        Serialize(bool, ShotSparks, false);
        Serialize(Color_t, ShotSparksColor, Color_t::White());
        Serialize(float, ShotSparksDensity, 500.f);
        Serialize(float, ShotSparksGravityZ, 0.f);
        Serialize(float, ShotSparksIntensity, 2.f);
        Serialize(float, ShotSparksGlowIntensity, 2.f);
        Serialize(float, ShotSparksSize, 0.1f);
        Serialize(float, ShotSparksLength, 0.05f);

        Serialize(Color_t, AshColor, Color_t::White());
        Serialize(float, AshDuration, 4.f);
        Serialize(float, AshDensity, 500.f);
        Serialize(float, AshChildDensity, 500.f);
        Serialize(float, AshGravityX, 0.f);
        Serialize(float, AshGravityY, 0.f);
        Serialize(float, AshGravityZ, -200.f);
    }

    namespace Movement {
        Serialize(bool, Bunnyhop, false);
        Serialize(bool, FastStop, false);
        Serialize(bool, JumpBug, false);
        Serialize(bool, Strafer, false);
        Serialize(bool, FastLadder, false);
        Serialize(bool, LegMovement, false);
        Serialize(bool, PeekAssist, false);
        Serialize(Color_t, PeekAssistColor, Color_t::White());
        Serialize(float, PeekAssistMaxDistance, 0.f);
        Serialize(bool, PeekAssistJumpOnRetreat, false);
        SerializeVector(int, PeekAssistTypes, std::vector<int>(peek_assist_types_max, 0));
        // --- advanced movement exploits ---
        Serialize(bool, Edgebug, false);
        Serialize(bool, Texturebug, false);
        Serialize(bool, Pixelsurf, false);
        Serialize(bool, Prediction, false);
    }

    namespace Grenades {
        Serialize(bool, Trajectory, false);
        Serialize(bool, PredictDamage, false);
        Serialize(int, PredictDamageFont, 0);
        Serialize(Color_t, PredictDamageColor1, Color_t(255, 60, 60, 255));
        Serialize(Color_t, PredictDamageColor2, Color_t(255, 255, 255, 255));
        Serialize(bool, TrajectoryPinPulled, false);
        Serialize(Color_t, TrajectoryColor, Color_t(255, 255, 255, 255));
        Serialize(Color_t, TrajectoryHitColor, Color_t(255, 60, 60, 255));
        Serialize(float, TrajectoryScale, 1.5f);
        Serialize(float, TrajectoryGlowIntensity, 0.4f);
        Serialize(bool, Warnings, false);
        Serialize(Color_t, WarningsColor, Color_t(255, 255, 255, 220));
    }

    namespace Misc {
        Serialize(bool, Watermark, true);
        Serialize(bool, WatermarkUsername, true);
        Serialize(bool, WatermarkPing, true);
        Serialize(bool, WatermarkFps, true);
        Serialize(bool, WatermarkSpeed, true);
        Serialize(bool, WatermarkTime, true);
        Serialize(int, WatermarkOpacity, 120);
        Serialize(Color_t, WatermarkAccentColor, Color_t(210, 150, 20, 122));
        Serialize(bool, BindsList, true);
        Serialize(Color_t, BindsListColor, Color_t(210, 150, 20, 122));
        Serialize(int, BindsListOpacity, 160);
        Serialize(float, BindsListPosX, -1.f);
        Serialize(float, BindsListPosY, -1.f);
        Serialize(bool, SpectatorList, true);
        Serialize(Color_t, SpectatorListColor, Color_t(210, 150, 20, 122));
        Serialize(int, SpectatorListOpacity, 160);
        Serialize(float, SpectatorListPosX, -1.f);
        Serialize(float, SpectatorListPosY, -1.f);

        Serialize(bool, Indicators, false);
        SerializeVector(int, IndicatorItems, std::vector<int>(8, 0));
        Serialize(int, IndicatorOpacity, 120);

        Serialize(bool, CrosshairIndicators, false);
        Serialize(Color_t, CrosshairIndicatorsColor, Color_t::White());
        Serialize(int, CrosshairIndicatorsOpacity, 255);

        Serialize(bool, QuickSwitch, false);
        Serialize(bool, BuyBot, false);
        Serialize(int, BuyBotPrimary, 0);
        Serialize(int, BuyBotSecondary, 0);
        Serialize(bool, BuyBotArmor, true);
        Serialize(bool, BuyBotDefuser, true);
        SerializeVector(int, BuyBotGrenades, std::vector<int>(4, 0));
        Serialize(bool, Hitmarker, false);
        Serialize(Color_t, HitmarkerColor, Color_t::White());
        Serialize(bool, Hitsound, false);
        Serialize(float, HitsoundVolume, 0.2f);
        Serialize(int, HitsoundBodyHit, 0);
        Serialize(int, HitsoundBodyKill, 0);
        Serialize(int, HitsoundHeadHit, 0);
        Serialize(int, HitsoundHeadKill, 0);
    }

    namespace Skins {
        Serialize(int, Agent, 0);
        Serialize(int, AgentT, 0);
        Serialize(int, AgentCT, 0);

        Serialize(int, Glove, 0);
        Serialize(int, GloveT, 0);
        Serialize(int, GloveCT, 0);
    }
}
