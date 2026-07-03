#include "../../../hooks/hooks.h"
#include "../../../config/config.h"
#include "../../../interfaces/interfaces.h"
#include "../../../../cs2/datatypes/cviewsetup/cviewsetup.h"

// CViewRender::GetMatricesForView - the game passes the render view setup here
// with the aim punch (visual recoil / camera kick) already baked into the view
// angles. Overriding them with the raw input angles BEFORE the matrices are
// computed removes the screen shake when shooting, without affecting where
// bullets actually go (server-side spread/recoil is untouched).
void __fastcall H::hkGetMatricesForView(void* rcx, CViewSetup* pSetup, void* pWorldToView, void* pViewToProjection, void* pWorldToProjection, void* pWorldToPixels)
{
	if (Config::no_visual_recoil && pSetup && I::Input)
	{
		C_CSPlayerPawn* pLocalPawn = H::oGetLocalPlayer ? H::oGetLocalPlayer(0) : nullptr;
		if (pLocalPawn && pLocalPawn->m_iHealth() > 0)
			pSetup->vecViewAngles = I::Input->GetViewAngles();
	}

	H::GetMatricesForView.GetOriginal()(rcx, pSetup, pWorldToView, pViewToProjection, pWorldToProjection, pWorldToPixels);
}
