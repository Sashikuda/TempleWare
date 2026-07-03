#pragma once
#include "../../../templeware/utils/memory/memorycommon.h"
#include "../../../templeware/utils/math/vector/vector.h"

// render view setup, passed as 2nd argument to CViewRender::GetMatricesForView.
// the game bakes the aim punch (visual recoil) into vecViewAngles before
// computing the render matrices, so overriding it here removes the camera kick.
class CViewSetup
{
public:
	MEM_PAD(0x4D8);            //0x0000
	float flFov;               //0x04D8
	float flViewmodelFov;      //0x04DC
	Vector_t vecOrigin;        //0x04E0
	MEM_PAD(0xC);              //0x04EC
	Vector_t vecViewAngles;    //0x04F8
	MEM_PAD(0x14);             //0x0504
	float flAspectRatio;       //0x0518
	MEM_PAD(0x1C);             //0x051C
}; //Size: 0x0538
static_assert(sizeof(CViewSetup) == 0x538, "CViewSetup size mismatch");
