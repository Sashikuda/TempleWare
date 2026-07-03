#include "skybox.h"

#include <Windows.h>
#include <atomic>
#include <cstdio>
#include <cstring>

#include "../../../interfaces/interfaces.h"
#include "../../../interfaces/CGameEntitySystem/CGameEntitySystem.h"
#include "../../../../cs2/entity/C_BaseEntity/C_BaseEntity.h"

// -----------------------------------------------------------------------------
// Version-specific references (reverse engineered - revalidate after game
// updates, values taken from the researched source snapshot / forum post).
// -----------------------------------------------------------------------------
namespace off
{
	// client.dll
	constexpr std::ptrdiff_t ResourceSystemGlobal = 0x25583E0;
	constexpr std::ptrdiff_t SkyStateChanged      = 0x266A60;

	// resourcesystem.dll
	constexpr std::ptrdiff_t ResourceNameBuilder  = 0x32350;

	// ResourceSystem virtual table byte offsets
	constexpr std::ptrdiff_t VT_BlockingLoad = 0x140;
	constexpr std::ptrdiff_t VT_Status       = 0x188;
	constexpr std::ptrdiff_t VT_Check        = 0x198;
	constexpr std::ptrdiff_t VT_Acquire      = 0x278;

	// ResourceSystem entry fields (these live in the resource entry, NOT C_EnvSky)
	constexpr std::ptrdiff_t EntryNameHolder = 0x08; // pointer to the name holder
	constexpr std::ptrdiff_t EntryFlags      = 0x18; // resource state / flags
	constexpr std::ptrdiff_t EntryTypeId     = 0x1C; // resource type identifier
	constexpr std::ptrdiff_t EntryRefCount   = 0x20; // reference counter

	// C_EnvSky
	constexpr std::ptrdiff_t m_hSkyMaterial             = 0xFA8;
	constexpr std::ptrdiff_t m_hSkyMaterialLightingOnly = 0xFB0;
	constexpr std::ptrdiff_t m_vTintColor               = 0xFB9;
	constexpr std::ptrdiff_t m_vTintColorLightingOnly   = 0xFBD;
	constexpr std::ptrdiff_t m_flBrightnessScale        = 0xFC4;

	// A resource entry is fully loaded when its status equals this value
	constexpr int ResourceStatusLoaded = 3;
}

// -----------------------------------------------------------------------------
// Engine function signatures (version-specific)
// -----------------------------------------------------------------------------
using ResourceNameBuilder_fn = void* (__fastcall*)(void* pOutName, const char* szResourcePath, std::uint64_t uUnk1, std::uint64_t uUnk2);
using SkyStateChanged_fn     = void  (__fastcall*)(void* pEnvSky);
using RSCheck_fn             = std::uintptr_t(__fastcall*)(void* pResourceSystem, void* pResourceName);
using RSAcquire_fn           = std::uintptr_t(__fastcall*)(void* pResourceSystem, void* pResourceName);
using RSStatus_fn            = int   (__fastcall*)(void* pResourceSystem, std::uintptr_t uEntry);
using RSBlockingLoad_fn      = void  (__fastcall*)(void* pResourceSystem, std::uintptr_t uEntry);

// -----------------------------------------------------------------------------
// Map profiles - skyname VMATs extracted from official maps.
// A profile is only applied when its VMAT resolves through the ResourceSystem
// with an exact name match, so unresolved entries are rejected safely.
// -----------------------------------------------------------------------------
static const skybox::sky_profile_t g_profiles[] = {
	{ "Mirage",   "de_mirage",   "materials/skybox/sky_de_mirage.vmat" },
	{ "Dust II",  "de_dust2",    "materials/skybox/sky_de_dust2.vmat" },
	{ "Inferno",  "de_inferno",  "materials/skybox/test/s2_de_inferno_sky01.vmat" },
	{ "Nuke",     "de_nuke",     "materials/skybox/sky_de_nuke.vmat" },
	{ "Ancient",  "de_ancient",  "materials/skybox/sky_de_ancient.vmat" },
	{ "Anubis",   "de_anubis",   "materials/skybox/sky_de_anubis.vmat" },
	{ "Overpass", "de_overpass", "materials/skybox/sky_de_overpass.vmat" },
	{ "Vertigo",  "de_vertigo",  "materials/skybox/sky_de_vertigo.vmat" },
	{ "Train",    "de_train",    "materials/skybox/sky_de_train.vmat" },
	{ "Italy",    "cs_italy",    "materials/skybox/sky_cs_italy.vmat" },
};
static const char* g_profileNames[] = {
	"Mirage", "Dust II", "Inferno", "Nuke", "Ancient",
	"Anubis", "Overpass", "Vertigo", "Train", "Italy",
};
static_assert(sizeof(g_profiles) / sizeof(g_profiles[0]) == sizeof(g_profileNames) / sizeof(g_profileNames[0]), "profile tables out of sync");

// -----------------------------------------------------------------------------
// State
// -----------------------------------------------------------------------------

// Original sky state captured when a new active C_EnvSky entity is observed.
struct captured_sky_t {
	bool          valid = false;
	void*         pSky = nullptr;      // entity we captured from
	char          szVmat[260]{};       // original material path
	std::uint8_t  tint[4]{};           // m_vTintColor (rgba bytes)
	std::uint8_t  tintLighting[4]{};   // m_vTintColorLightingOnly (rgba bytes)
	float         flBrightness = 1.0f; // m_flBrightnessScale
};

static captured_sky_t   g_captured;
static std::atomic_bool g_pendingApply{ false };
static std::atomic_bool g_pendingReset{ false };
static char             g_status[128] = "Idle";

static void set_status(const char* fmt, const char* arg = nullptr)
{
	if (arg)
		std::snprintf(g_status, sizeof(g_status), fmt, arg);
	else
		std::snprintf(g_status, sizeof(g_status), "%s", fmt);
}

// -----------------------------------------------------------------------------
// ResourceSystem helpers
// -----------------------------------------------------------------------------

template <typename Fn>
static Fn vfunc_at(void* pThis, std::ptrdiff_t nVtableByteOffset)
{
	const auto uVtable = *reinterpret_cast<std::uintptr_t*>(pThis);
	return *reinterpret_cast<Fn*>(uVtable + nVtableByteOffset);
}

static void* resource_system()
{
	const auto hClient = reinterpret_cast<std::uintptr_t>(GetModuleHandleA("client.dll"));
	if (!hClient)
		return nullptr;

	return *reinterpret_cast<void**>(hClient + off::ResourceSystemGlobal);
}

static SkyStateChanged_fn sky_state_changed()
{
	const auto hClient = reinterpret_cast<std::uintptr_t>(GetModuleHandleA("client.dll"));
	if (!hClient)
		return nullptr;

	return reinterpret_cast<SkyStateChanged_fn>(hClient + off::SkyStateChanged);
}

static ResourceNameBuilder_fn resource_name_builder()
{
	const auto hResourceSystem = reinterpret_cast<std::uintptr_t>(GetModuleHandleA("resourcesystem.dll"));
	if (!hResourceSystem)
		return nullptr;

	return reinterpret_cast<ResourceNameBuilder_fn>(hResourceSystem + off::ResourceNameBuilder);
}

// Reads the resolved name of a resource entry through its name holder.
static const char* entry_name(std::uintptr_t uEntry)
{
	if (!uEntry)
		return nullptr;

	const char* szName = *reinterpret_cast<const char**>(uEntry + off::EntryNameHolder);
	return szName;
}

static void entry_addref(std::uintptr_t uEntry)
{
	if (uEntry)
		InterlockedIncrement(reinterpret_cast<volatile LONG*>(uEntry + off::EntryRefCount));
}

static void entry_release(std::uintptr_t uEntry)
{
	if (uEntry)
		InterlockedDecrement(reinterpret_cast<volatile LONG*>(uEntry + off::EntryRefCount));
}

// Resolves a VMAT path to a loaded ResourceSystem entry.
// A candidate is only accepted when it is fully loaded and its resolved name
// matches the requested VMAT exactly.
static std::uintptr_t resolve_vmat(const char* szVmatPath)
{
	void* pResourceSystem = resource_system();
	const auto fnBuildName = resource_name_builder();
	if (!pResourceSystem || !fnBuildName || !szVmatPath || !szVmatPath[0])
		return 0;

	// build the ResourceSystem name for the requested path
	alignas(16) std::uint8_t nameBuffer[0x100]{};
	fnBuildName(nameBuffer, szVmatPath, 0, 0);

	// check for an existing entry, acquire one otherwise
	std::uintptr_t uEntry = vfunc_at<RSCheck_fn>(pResourceSystem, off::VT_Check)(pResourceSystem, nameBuffer);
	if (!uEntry)
		uEntry = vfunc_at<RSAcquire_fn>(pResourceSystem, off::VT_Acquire)(pResourceSystem, nameBuffer);

	if (!uEntry)
		return 0;

	// blocking load when the entry is not loaded yet (status != 3)
	const auto fnStatus = vfunc_at<RSStatus_fn>(pResourceSystem, off::VT_Status);
	if (fnStatus(pResourceSystem, uEntry) != off::ResourceStatusLoaded)
		vfunc_at<RSBlockingLoad_fn>(pResourceSystem, off::VT_BlockingLoad)(pResourceSystem, uEntry);

	if (fnStatus(pResourceSystem, uEntry) != off::ResourceStatusLoaded)
		return 0;

	// exact resolved-name match is mandatory
	const char* szResolved = entry_name(uEntry);
	if (!szResolved || _stricmp(szResolved, szVmatPath) != 0)
		return 0;

	return uEntry;
}

// -----------------------------------------------------------------------------
// Sky entity helpers
// -----------------------------------------------------------------------------

// The active C_EnvSky entity is found by walking every client entity down to
// its identity record and comparing the designer name against env_sky.
static C_BaseEntity* find_env_sky()
{
	if (!I::GameEntity || !I::GameEntity->Instance)
		return nullptr;

	const int nHighest = I::GameEntity->Instance->GetHighestEntityIndex();
	for (int i = 1; i <= nHighest; i++)
	{
		auto pEntity = I::GameEntity->Instance->Get(i);
		if (!pEntity)
			continue;

		CEntityIdentity* pIdentity = pEntity->m_pEntityIdentity();
		if (!pIdentity)
			continue;

		const char* szDesigner = pIdentity->m_designerName();
		if (!szDesigner)
			continue;

		if (_stricmp(szDesigner, "env_sky") == 0 || _stricmp(szDesigner, "C_EnvSky") == 0)
			return pEntity;
	}

	return nullptr;
}

static std::uintptr_t* sky_material_slot(void* pSky)
{
	return reinterpret_cast<std::uintptr_t*>(reinterpret_cast<std::uintptr_t>(pSky) + off::m_hSkyMaterial);
}

// Reads the material path currently installed in the sky's strong-handle slot.
static const char* active_sky_material_name(void* pSky)
{
	const std::uintptr_t uEntry = *sky_material_slot(pSky);
	return entry_name(uEntry);
}

// Captures the original material path and environment values of a newly
// observed sky entity so Reset can restore them later.
static void capture_original(void* pSky)
{
	g_captured = {};

	const char* szActive = active_sky_material_name(pSky);
	if (!szActive || !szActive[0])
		return;

	const auto uSky = reinterpret_cast<std::uintptr_t>(pSky);

	g_captured.pSky = pSky;
	std::snprintf(g_captured.szVmat, sizeof(g_captured.szVmat), "%s", szActive);
	std::memcpy(g_captured.tint, reinterpret_cast<void*>(uSky + off::m_vTintColor), 4);
	std::memcpy(g_captured.tintLighting, reinterpret_cast<void*>(uSky + off::m_vTintColorLightingOnly), 4);
	g_captured.flBrightness = *reinterpret_cast<float*>(uSky + off::m_flBrightnessScale);
	g_captured.valid = true;
}

// -----------------------------------------------------------------------------
// Strong-handle material transaction
//
// Ownership order matters: addref the new entry BEFORE touching the slot,
// then release the previous entry only once the write succeeded. If the slot
// write fails validation, the previous entry is kept, re-written, and the new
// reference is released.
// -----------------------------------------------------------------------------
static bool install_material(void* pSky, std::uintptr_t uNewEntry, const char* szExpectedVmat)
{
	if (!pSky || !uNewEntry)
		return false;

	const auto fnSkyStateChanged = sky_state_changed();
	if (!fnSkyStateChanged)
		return false;

	std::uintptr_t* pSlot = sky_material_slot(pSky);

	// read the current C_EnvSky material entry
	const std::uintptr_t uPrevEntry = *pSlot;

	// hold the requested entry, replace the strong-handle slot
	entry_addref(uNewEntry);
	*pSlot = uNewEntry;

	// propagate the new state to every render-dependent object -
	// swapping the handle alone is not enough
	fnSkyStateChanged(pSky);

	// re-read the active material name to validate the transaction
	const char* szActive = active_sky_material_name(pSky);
	if (!szActive || _stricmp(szActive, szExpectedVmat) != 0)
	{
		// rollback: keep the previous entry, restore the slot, drop our reference
		*pSlot = uPrevEntry;
		fnSkyStateChanged(pSky);
		entry_release(uNewEntry);
		return false;
	}

	// success: release the previous entry
	entry_release(uPrevEntry);
	return true;
}

// Writes tint / brightness onto the active sky entity and re-propagates.
static void apply_environment(void* pSky, const std::uint8_t tint[4], const std::uint8_t tintLighting[4], float flBrightness)
{
	const auto uSky = reinterpret_cast<std::uintptr_t>(pSky);

	std::memcpy(reinterpret_cast<void*>(uSky + off::m_vTintColor), tint, 4);
	std::memcpy(reinterpret_cast<void*>(uSky + off::m_vTintColorLightingOnly), tintLighting, 4);
	*reinterpret_cast<float*>(uSky + off::m_flBrightnessScale) = flBrightness;

	if (const auto fnSkyStateChanged = sky_state_changed())
		fnSkyStateChanged(pSky);
}

// -----------------------------------------------------------------------------
// Transactions (game thread)
// -----------------------------------------------------------------------------

static void do_apply(void* pSky)
{
	const int nIndex = features::skybox_cfg.selected_map;
	if (nIndex < 0 || nIndex >= skybox::map_count())
	{
		set_status("Apply rejected: invalid selection");
		return;
	}

	const skybox::sky_profile_t& profile = g_profiles[nIndex];

	const std::uintptr_t uEntry = resolve_vmat(profile.szSkyVmat);
	if (!uEntry)
	{
		set_status("Apply rejected: failed to resolve %s", profile.szSkyVmat);
		return;
	}

	if (!install_material(pSky, uEntry, profile.szSkyVmat))
	{
		set_status("Apply rejected: transaction failed for %s", profile.szSkyVmat);
		return;
	}

	// optional environment overrides on top of the installed material
	const sky_config_t& cfg = features::skybox_cfg;
	std::uint8_t tint[4];
	std::memcpy(tint, reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(pSky) + off::m_vTintColor), 4);
	if (cfg.override_tint)
	{
		tint[0] = static_cast<std::uint8_t>(cfg.tint.x * 255.0f);
		tint[1] = static_cast<std::uint8_t>(cfg.tint.y * 255.0f);
		tint[2] = static_cast<std::uint8_t>(cfg.tint.z * 255.0f);
		tint[3] = static_cast<std::uint8_t>(cfg.tint.w * 255.0f);
	}

	std::uint8_t tintLighting[4];
	std::memcpy(tintLighting, reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(pSky) + off::m_vTintColorLightingOnly), 4);

	const float flBrightness = cfg.override_brightness
		? cfg.brightness
		: *reinterpret_cast<float*>(reinterpret_cast<std::uintptr_t>(pSky) + off::m_flBrightnessScale);

	apply_environment(pSky, tint, tintLighting, flBrightness);

	set_status("Applied %s", profile.szDisplayName);
}

static void do_reset(void* pSky)
{
	if (!g_captured.valid || g_captured.pSky != pSky)
	{
		set_status("Reset rejected: no captured original for this sky");
		return;
	}

	// the original material resolves through the same ResourceSystem route
	const std::uintptr_t uEntry = resolve_vmat(g_captured.szVmat);
	if (!uEntry)
	{
		set_status("Reset rejected: failed to resolve %s", g_captured.szVmat);
		return;
	}

	if (!install_material(pSky, uEntry, g_captured.szVmat))
	{
		set_status("Reset rejected: transaction failed");
		return;
	}

	apply_environment(pSky, g_captured.tint, g_captured.tintLighting, g_captured.flBrightness);

	set_status("Restored original sky");
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

const char* const* skybox::map_names() { return g_profileNames; }
int skybox::map_count() { return static_cast<int>(sizeof(g_profiles) / sizeof(g_profiles[0])); }

void skybox::request_apply() { g_pendingApply.store(true); }
void skybox::request_reset() { g_pendingReset.store(true); }

const char* skybox::status() { return g_status; }

void skybox::on_frame()
{
	if (!I::EngineClient || !I::EngineClient->valid())
	{
		// out of game: drop the capture, it points at a dead entity
		g_captured = {};
		g_pendingApply.store(false);
		g_pendingReset.store(false);
		return;
	}

	C_BaseEntity* pSky = find_env_sky();
	if (!pSky)
	{
		g_captured = {};
		return;
	}

	// a new active sky entity was observed: capture its original state
	if (!g_captured.valid || g_captured.pSky != pSky)
		capture_original(pSky);

	if (g_pendingApply.exchange(false))
		do_apply(pSky);

	if (g_pendingReset.exchange(false))
		do_reset(pSky);
}
