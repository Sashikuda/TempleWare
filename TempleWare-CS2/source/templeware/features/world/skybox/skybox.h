#pragma once
#include <cstdint>
#include "../../../../../external/imgui/imgui.h"

// -----------------------------------------------------------------------------
// Skybox Changer
//
// Swaps the active C_EnvSky 2D sky material by installing a resolved
// ResourceSystem entry (strong handle / HMaterialStrong) into m_hSkyMaterial
// instead of writing a raw CMaterial2 pointer, then propagates the change via
// SkyStateChanged. Also applies tint / brightness and supports a full Reset
// back to the captured original state.
//
// Execution flow (see forum post):
//   UI selection -> map profile -> resolve VMAT via ResourceSystem
//     -> addref new entry -> write C_EnvSky strong-handle slot
//     -> release previous entry -> SkyStateChanged
//     -> apply tint / brightness
//
// Apply / Reset are revision-driven UI operations: the transaction is only
// submitted when the user requests a change, never once per rendered frame.
// -----------------------------------------------------------------------------

struct sky_config_t {
	int    selected_map = 0;
	bool   override_tint = false;
	ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	bool   override_brightness = false;
	float  brightness = 1.0f;
};

namespace skybox
{
	// Map profile shown in the UI: display name + official map name + skyname VMAT.
	// The UI itself only stores display/map names, never resolved VMAT paths.
	struct sky_profile_t {
		const char* szDisplayName;  // e.g. "Mirage"
		const char* szMapName;      // e.g. "de_mirage"
		const char* szSkyVmat;      // e.g. "materials/skybox/sky_de_mirage.vmat"
	};

	// UI helpers
	const char* const* map_names();
	int map_count();

	// Revision-driven requests (called from the menu / UI thread)
	void request_apply();
	void request_reset();

	// Human readable result of the last transaction (for the menu)
	const char* status();

	// Runs on the game thread (FrameStageNotify / FRAME_RENDER_END).
	// Captures the original sky state when a new active C_EnvSky entity is
	// observed and processes any pending Apply / Reset transaction.
	void on_frame();
}

namespace features {
	inline sky_config_t skybox_cfg;
}
