#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <intrin.h>
#include <xmmintrin.h>
#include <array>
#include <vector>
#include <algorithm>
#include <cctype>
#include <thread>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <deque>
#include <functional>
#include <map>
#include <shlobj.h>
#include <shellapi.h>
#include <filesystem>
#include <streambuf>
#include <mutex>
#include <strsafe.h>
#include <Psapi.h>
#include <dxgi.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <cmath>
#include <shared_mutex>
#include <any>
#include <ranges>

#undef min
#undef max

#include "xorstring/xorstring.h"
#include "darkhook/include/darkhook.hpp"

#include "../SDK/Protobuf/cs_usercmd.pb.h"
#include "../SDK/Protobuf/usercmd.pb.h"
#include "../SDK/Protobuf/networkbasetypes.pb.h"
#include "../SDK/Protobuf/Proto.h"

#include "../Utils/Math/Math.h"
#include "../Utils/Memory/Memory.h"
#include "../Utils/Fnv1a/Fnv1a.h"
#include "../Utils/Murmur/Murmur.h"

#include "../Utils/.vsnd/vsnd.h"
#include "../Utils/.vpcf/vpcf.h"

#include "../SDK/Valve/Datatypes/UtlVec.h"
#include "../SDK/Valve/Datatypes/UtlBuffer.h"
#include "../SDK/Valve/Datatypes/UtlString.h"
#include "../SDK/Valve/Datatypes/UtlMap.h"
#include "../SDK/Valve/Datatypes/Taia.h"
#include "../SDK/Valve/Datatypes/UserCommand.h"
#include "../SDK/Valve/Datatypes/CSGOInput.h"

#include "../Gui/Settings/Colors.h"
#include "../Gui/Settings/Functions.h"

#include "../SDK/Globals.h"

#include "../Configurations/Configurations.h"

#include "../SDK/Valve/Interfaces/SchemaSystem.h"

#include "../Utils/Schema/Schema.h"

#include "../SDK/Valve/Fields/Handle.h"

#include "../SDK/Valve/Interfaces/ViewSetup.h"
#include "../SDK/Valve/Interfaces/Cvar.h"
#include "../SDK/Valve/Interfaces/EntitySystem.h"
#include "../SDK/Valve/Interfaces/Physics.h"
#include "../SDK/Valve/Interfaces/GlobalVars.h"
#include "../SDK/Valve/Interfaces/Prediction.h"
#include "../SDK/Valve/Interfaces/Swapchain.h"
#include "../SDK/Valve/Interfaces/KeyValues.h"
#include "../SDK/Valve/Interfaces/MaterialSystem.h"
#include "../SDK/Valve/Interfaces/SceneObject.h"
#include "../SDK/Valve/Interfaces/EngineClient.h"
#include "../SDK/Valve/Interfaces/EventSystem.h"
#include "../SDK/Valve/Interfaces/ParticleSystem.h"
#include "../SDK/Valve/Interfaces/ResourceSystem.h"
#include "../SDK/Valve/Interfaces/EconItem.h"
#include "../SDK/Valve/Interfaces/SourceClient.h"
#include "../SDK/Valve/Interfaces/Localize.h"
#include "../SDK/Valve/Interfaces/Filesystem.h"
#include "../SDK/Valve/Interfaces/EnginePanorama.h"

#include "../SDK/Valve/Fields/Entity.h"
#include "../SDK/Valve/Fields/Controller.h"
#include "../SDK/Valve/Fields/Move.h"

#include "../SDK/Valve/EntityList/EntityList.h"

#include "../Utils/Logger/Logger.h"

#include "../SDK/Hooks/Hooks.h"

#include "../SDK/Valve/Datatypes/Enums.h"

#include "../Features/Prediction/Prediction.h"
#include "../Features/Movement/Movement.h"
#include "../Features/Ragebot/Autowall.h"
#include "../Features/Ragebot/LagCompensation.h"
#include "../Features/Ragebot/Ragebot.h"
#include "../Features/Misc/Misc.h"
#include "../Features/Player/Player.h"
#include "../Features/Grenade/Trajectory/Trajectory.h"
#include "../Features/Grenade/Warnings/Warnings.h"
#include "../Features/World/World.h"
#include "../Features/AntiAim/AntiAim.h"
#include "../Features/ColoredModels/ColoredModels.h"
#include "../Features/Events/Events.h"
#include "../Features/Particles/Particles.h"
#include "../Features/Skins/Skins.h"