#include "movement.h"

void Movement::Bhop(CUserCmd* user_cmd)
{
    C_CSPlayerPawn* pLocalPawn = H::oGetLocalPlayer(0);

    if (!pLocalPawn || pLocalPawn->m_iHealth() <= 0)
        return;

    bool jumping = user_cmd->nButtons.nValue & IN_JUMP || user_cmd->nButtons.nValueScroll & IN_JUMP;

    if (!jumping)
        return;

    user_cmd->nButtons.nValue &= ~IN_JUMP;
    user_cmd->nButtons.nValueScroll &= ~IN_JUMP;
    user_cmd->nButtons.nValueChanged &= ~IN_JUMP;

    if (!(pLocalPawn->m_fFlags() & FL_ONGROUND))
        return;

    user_cmd->nButtons.nValue |= IN_JUMP;
    user_cmd->nButtons.nValueScroll |= IN_JUMP;
}

void Movement::OnCreateMove(CUserCmd* user_cmd)
{
	if (!H::oGetLocalPlayer(0) || !user_cmd || !I::EngineClient->in_game() || !I::EngineClient->connected())
		return;

	if (Config::bhop)
		Bhop(user_cmd);
}
