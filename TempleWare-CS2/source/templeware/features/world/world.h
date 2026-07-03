#pragma once
#include "../../../cs2/entity/C_AggregateSceneObject/C_AggregateSceneObject.h"

// -----------------------------------------------------------------------------
// World / Smoke color modulation
//
// World color:
//   Hooks scenesystem.dll DrawAggregateSceneObjectArray and overwrites the
//   per-object entries in the scene light data queue with a user color, tinting
//   the whole world geometry. Requires the SceneSystem interface for the light
//   data queue pointer.
//
// Smoke color:
//   Walks the client entity list every frame and writes m_vSmokeColor on every
//   smokegrenade_projectile so active smokes render with the chosen color.
// -----------------------------------------------------------------------------
namespace world
{
	// Resolves the SceneSystem interface used for the world color hook.
	// Call once during hook initialization (scenesystem.dll must be loaded).
	void init();

	// Runs on the game thread (FrameStageNotify / FRAME_RENDER_END).
	// Applies the smoke color to every active smoke grenade projectile.
	void on_frame();

	// Detour for scenesystem.dll DrawAggregateSceneObjectArray.
	void* __fastcall hook(void* a1, void* a2, C_AggregateSceneObject* data);
}
