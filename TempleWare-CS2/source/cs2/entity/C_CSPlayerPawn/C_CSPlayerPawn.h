#pragma once
#include "../../../templeware/utils/memory/memorycommon.h"
#include "../../../templeware/utils/math/vector/vector.h"
#include "../../../templeware/utils/schema/schema.h"
#include "../C_CSWeaponBase/C_CSWeaponBase.h"
#include "../C_BaseEntity/C_BaseEntity.h"

#include <cstdint>

class ViewAngleServerChange_t {
public:
	schema(QAngle_t*, qAngle, "ViewAngleServerChange_t->qAngle");
};

class C_CSPlayerPawn : public C_BaseEntity {
public:
	schema(Vector_t, m_vOldOrigin, "C_BasePlayerPawn->m_vOldOrigin");
	schema(Vector_t, m_vecViewOffset, "C_BaseModelEntity->m_vecViewOffset");
	schema(CCSPlayer_WeaponServices*, m_pWeaponServices, "C_BasePlayerPawn->m_pWeaponServices");
	schema(ViewAngleServerChange_t*, m_ServerViewAngleChanges, "C_BasePlayerPawn->m_ServerViewAngleChanges");
	C_CSPlayerPawn(uintptr_t address);

	C_CSWeaponBase* GetActiveWeapon()const;
	CCSPlayer_WeaponServices* GetWeaponServices()const;
	Vector_t getPosition() const;
	Vector_t getEyePosition() const;

	uintptr_t getAddress() const;
	int getHealth() const;
	uint8_t getTeam() const;
	Vector_t getViewOffset() const;
private:
	uintptr_t address;
};
