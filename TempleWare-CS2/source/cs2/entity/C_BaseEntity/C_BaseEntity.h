#pragma once
#include <cstdint>
#include "../C_EntityInstance/C_EntityInstance.h"
#include "../../../templeware/utils/memory/memorycommon.h"
#include "../../../templeware/utils/math/vector/vector.h"
#include "../../../../source/templeware/utils/schema/schema.h"
#include "../../../../source/templeware/utils/memory/vfunc/vfunc.h"
#include "../handle.h"

class C_BaseEntity : public CEntityInstance
{
public:
	schema(int, m_iMaxHealth, "C_BaseEntity->m_iMaxHealth");
	schema(std::int32_t, m_iHealth, "C_BaseEntity->m_iHealth");
	schema(std::uint8_t, m_iTeamNum, "C_BaseEntity->m_iTeamNum");
	schema(std::uint32_t, m_fFlags, "C_BaseEntity->m_fFlags");
	schema(CBaseHandle, m_hController, "C_BasePlayerPawn->m_hController");
	bool IsBasePlayer()
	{
		SchemaClassInfoData_t* pClassInfo;
		dump_class_info(&pClassInfo);
		if (pClassInfo == nullptr)
			return false;

		return hash_32_fnv1a_const(pClassInfo->szName) == hash_32_fnv1a_const("C_CSPlayerPawn");
	}

	bool IsViewmodelAttachment()
	{
		SchemaClassInfoData_t* pClassInfo;
		dump_class_info(&pClassInfo);
		if (pClassInfo == nullptr)
			return false;

		return hash_32_fnv1a_const(pClassInfo->szName) == hash_32_fnv1a_const("C_CS2HudModelArms");
	}

	bool IsViewmodel()
	{
		SchemaClassInfoData_t* pClassInfo;
		dump_class_info(&pClassInfo);
		if (pClassInfo == nullptr)
			return false;

		return hash_32_fnv1a_const(pClassInfo->szName) == hash_32_fnv1a_const("C_CS2HudModelWeapon");
	}

	bool IsPlayerController()
	{
		SchemaClassInfoData_t* _class = nullptr;
		dump_class_info(&_class);
		if (!_class)
			return false;

		const uint32_t hash = hash_32_fnv1a_const(_class->szName);

		return (hash == hash_32_fnv1a_const("CCSPlayerController"));
	}
};
