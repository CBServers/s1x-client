#pragma once

#include "structs.hpp"

namespace dvars
{
	extern game::dvar_t* aimassist_enabled;

	extern game::dvar_t* con_inputBoxColor;
	extern game::dvar_t* con_inputHintBoxColor;
	extern game::dvar_t* con_outputBarColor;
	extern game::dvar_t* con_outputSliderColor;
	extern game::dvar_t* con_outputWindowColor;
	extern game::dvar_t* con_inputDvarMatchColor;
	extern game::dvar_t* con_inputDvarValueColor;
	extern game::dvar_t* con_inputDvarInactiveValueColor;
	extern game::dvar_t* con_inputCmdMatchColor;

	extern game::dvar_t* g_playerCollision;
	extern game::dvar_t* g_playerEjection;

	extern game::dvar_t* bg_surfacePenetration;

	extern game::dvar_t* pm_bouncing;
	extern game::dvar_t* g_dump_scripts;
	extern game::dvar_t* g_gravity;
	extern game::dvar_t* g_speed;
	extern game::dvar_t* g_elevators;
	extern game::dvar_t* g_log;

	extern game::dvar_t* jump_height;
	extern game::dvar_t* jump_ladderPushVel;

	extern game::dvar_t* player_sustainAmmo;

	extern game::dvar_t* r_fullbright;

	extern game::dvar_t* cg_legacyCrashHandling;

	extern game::dvar_t* sv_cheats;

	std::string dvar_get_vector_domain(const int components, const game::dvar_limits& domain);
	std::string dvar_get_domain(const game::dvar_type type, const game::dvar_limits& domain);
}
