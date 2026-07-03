#include "hooks.h"
#include <iostream>

#include "../../../external/kiero/minhook/include/MinHook.h"

#include "../../templeware/utils/memory/Interface/Interface.h"
#include "../utils/memory/patternscan/patternscan.h"
#include "../utils/memory/gaa/gaa.h"

#include "../players/hook/playerHook.h"
#include "../features/visuals/visuals.h"
#include "../features/chams/chams.h"

#include "../../cs2/datatypes/cutlbuffer/cutlbuffer.h"
#include "../../cs2/datatypes/keyvalues/keyvalues.h"
#include "../../cs2/entity/C_Material/C_Material.h"

#include "../config/config.h"
#include "../interfaces/interfaces.h"
#include "../features/aim/aim.h"
#include "../features/movement/movement.h"

void __fastcall H::hkFrameStageNotify(void* a1, int stage)
{
	FrameStageNotify.GetOriginal()(a1, stage);

	// frame_render_stage | 9
	if (stage == FRAME_RENDER_END && oGetLocalPlayer(0)) {
		Esp::cache();

		Aimbot();
	}
}

void* __fastcall H::hkLevelInit(__int64 a1, __int64 a2) {
	const auto original = H::LevelInit.GetOriginal();

	static void* g_pPVS = (void*)M::getAbsoluteAddress(M::patternScan("engine2", "48 8D 0D ? ? ? ? 33 D2 FF 50"), 0x3);

	M::vfunc<void*, 6U, void>(g_pPVS, false);

	return original(a1, a2);
}

void __fastcall H::hkCreateMove(CCSGOInput* rcx, int slot, bool active)
{
	static auto original = CreateMove.GetOriginal();
	original(rcx, slot, active);

	C_CSPlayerPawn* pLocalPawn = H::oGetLocalPlayer(0);
	if (!pLocalPawn || pLocalPawn->m_iHealth() <= 0)
		return;

	CCSPlayerController* pLocalController = I::GameEntity->Instance->Get<CCSPlayerController>(pLocalPawn->m_hController().index());
	if (!pLocalController)
		return;

	CUserCmd* user_cmd = I::Input->get_user_cmd(pLocalController);

	g_movement->OnCreateMove(user_cmd);
}

void H::Hooks::init() {

	oGetWeaponData = *reinterpret_cast<int*>(M::patternScan("client", ("48 8B 81 ? ? ? ? 85 D2 78 ? 48 83 FA ? 73 ? F3 0F 10 84 90 ? ? ? ? C3 F3 0F 10 80 ? ? ? ? C3 CC CC CC CC")) + 0x3);
	ogGetBaseEntity = reinterpret_cast<decltype(ogGetBaseEntity)>(M::patternScan("client", ("4C 8D 49 10 81 FA FE 7F 00 00 ? ? 8B CA C1 F9 09 83 F9 3F ? ? 48 63 C1 4D"))); // String: Found no entity at %d.\n and Press Double Click on v4 and then on Return.
	oGetLocalPlayer = reinterpret_cast<decltype(oGetLocalPlayer)>(M::getAbsoluteAddress(M::patternScan("client", "e8 ? ? ? ? 48 8b f8 48 85 c0 0f 84 ? ? ? ? 48 8b 10 48 8b c8 ff 92 ? ? ? ? 84 c0 0f 84 ? ? ? ? 48 8b 17 48 8b cf ff 92 ? ? ? ? 84 c0 0f 84 ? ? ? ? 48 8b 07"), 1)); // Under Autobuy "; STR:

	if (I::Input)
	{
		using CreateMoveFn = void(__fastcall*)(CCSGOInput*, int, bool);

		auto createMoveFunc = M::GetVFunc<CreateMoveFn>(I::Input, 5);

		if (createMoveFunc)
		{
			if (CreateMove.Add(reinterpret_cast<void*>(createMoveFunc),
				reinterpret_cast<void*>(&hkCreateMove)))
			{
			}
		}
	}

	FrameStageNotify.Add((void*)M::patternScan("client", ("48 89 5C 24 ? 48 89 6C 24 ? 57 48 83 EC 40 48 8B F9 33 ED")), &hkFrameStageNotify);
	DrawArray.Add((void*)M::patternScan("scenesystem", ("48 8B C4 53 57 41 54 48 81 EC D0 00 00 00 49 63 F9 49")), &chams::hook);
	GetRenderFov.Add((void*)M::patternScan("client", "40 53 48 83 EC ? 48 8B D9 E8 ? ? ? ? 48 85 C0 74 ? 48 8B C8 48 83 C4"), &hkGetRenderFov);
	LevelInit.Add((void*)M::patternScan("client", "40 55 56 41 56 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 0D"), &hkLevelInit);
	RenderFlashBangOverlay.Add((void*)M::patternScan("client", ("85 D2 0F 88 ? ? ? ? 48 89 4C 24 ? 55 56")), &hkRenderFlashbangOverlay);

	MH_EnableHook(MH_ALL_HOOKS);
}
