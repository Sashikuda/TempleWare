#include "world.h"

#include <Windows.h>
#include <cstdint>
#include <cstring>

#include "../../config/config.h"
#include "../../hooks/hooks.h"
#include "../../interfaces/interfaces.h"
#include "../../interfaces/CGameEntitySystem/CGameEntitySystem.h"
#include "../../../cs2/entity/C_BaseEntity/C_BaseEntity.h"
#include "../../utils/math/vector/vector.h"
#include "../../utils/fnv1a/fnv1a.h"
#include "../../utils/schema/schema.h"

// -----------------------------------------------------------------------------
// SceneSystem layout (version-specific, revalidate after game updates)
// -----------------------------------------------------------------------------
namespace
{
	// 3 byte color written into the scene light data entries.
	struct Color3
	{
		std::uint8_t red, green, blue;
	};

	class CLightDataQueue
	{
	public:
		char  pad_0000[0x18];
		void* pLightData;
	};

	class CSceneSystem
	{
	public:
		char             pad_0000[0x2A28];
		CLightDataQueue* pLightDataQueue;
	};

	CSceneSystem* g_sceneSystem = nullptr;
}

// -----------------------------------------------------------------------------
// SceneSystem interface resolution
// -----------------------------------------------------------------------------
void world::init()
{
	HMODULE hScene = GetModuleHandleA("scenesystem.dll");
	if (!hScene)
		return;

	using CreateInterface_fn = void* (__cdecl*)(const char*, int*);
	auto fnCreateInterface = reinterpret_cast<CreateInterface_fn>(GetProcAddress(hScene, "CreateInterface"));
	if (!fnCreateInterface)
		return;

	g_sceneSystem = reinterpret_cast<CSceneSystem*>(fnCreateInterface("SceneSystem_002", nullptr));
}

// -----------------------------------------------------------------------------
// World color hook: overwrites the per-object light data entries after the
// original call has populated them.
// -----------------------------------------------------------------------------
void* __fastcall world::hook(void* a1, void* a2, C_AggregateSceneObject* data)
{
	auto original = H::DrawAggregate.GetOriginal();
	auto result = original(a1, a2, data);

	if (!Config::modulateWorld)
		return result;

	if (!g_sceneSystem || !g_sceneSystem->pLightDataQueue || !data || !data->data)
		return result;

	void* pLightData = g_sceneSystem->pLightDataQueue->pLightData;
	if (!pLightData)
		return result;

	const Color3 color{
		static_cast<std::uint8_t>(Config::worldColor.x * 255.0f),
		static_cast<std::uint8_t>(Config::worldColor.y * 255.0f),
		static_cast<std::uint8_t>(Config::worldColor.z * 255.0f),
	};

	for (int i = 0; i < data->data->count; i++)
	{
		int index = data->data->index + i;
		index = index << 5;
		*reinterpret_cast<Color3*>(reinterpret_cast<std::uintptr_t>(pLightData) + index) = color;
	}

	return result;
}

// -----------------------------------------------------------------------------
// Smoke color: applied per frame to every smoke grenade projectile.
// -----------------------------------------------------------------------------
void world::on_frame()
{
	if (!Config::modulateSmoke)
		return;

	if (!I::EngineClient || !I::EngineClient->valid())
		return;

	if (!I::GameEntity || !I::GameEntity->Instance)
		return;

	// schema offset of C_SmokeGrenadeProjectile->m_vSmokeColor (resolved once)
	static const std::uint32_t uSmokeColorOffset =
		SchemaFinder::Get(hash_32_fnv1a_const("C_SmokeGrenadeProjectile->m_vSmokeColor"));
	if (!uSmokeColorOffset)
		return;

	const int nHighest = I::GameEntity->Instance->GetHighestEntityIndex();
	for (int i = 1; i <= nHighest; i++)
	{
		C_BaseEntity* pEntity = I::GameEntity->Instance->Get(i);
		if (!pEntity)
			continue;

		CEntityIdentity* pIdentity = pEntity->m_pEntityIdentity();
		if (!pIdentity)
			continue;

		const char* szDesigner = pIdentity->m_designerName();
		if (!szDesigner)
			continue;

		if (std::strcmp(szDesigner, "smokegrenade_projectile") != 0)
			continue;

		Vector_t* pSmokeColor = reinterpret_cast<Vector_t*>(reinterpret_cast<std::uintptr_t>(pEntity) + uSmokeColorOffset);
		pSmokeColor->x = Config::smokeColor.x * 255.0f;
		pSmokeColor->y = Config::smokeColor.y * 255.0f;
		pSmokeColor->z = Config::smokeColor.z * 255.0f;
	}
}
