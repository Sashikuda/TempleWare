#pragma once
#include <memory>
#include "../../interfaces/CUserCmd/CUserCmd.h"
#include "../../hooks/hooks.h"
#include "../../interfaces/interfaces.h"
#include "../../config/config.h"

class Movement {
private:
	void Bhop(CUserCmd* user_cmd);

public:
	void OnCreateMove(CUserCmd* user_cmd);
};

const auto g_movement = std::make_unique<Movement>();