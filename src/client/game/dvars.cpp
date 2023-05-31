#include <std_include.hpp>
#include <utils/string.hpp>

#include "game.hpp"

namespace dvars
{
	game::dvar_t* aimassist_enabled = nullptr;

	game::dvar_t* con_inputBoxColor = nullptr;
	game::dvar_t* con_inputHintBoxColor = nullptr;
	game::dvar_t* con_outputBarColor = nullptr;
	game::dvar_t* con_outputSliderColor = nullptr;
	game::dvar_t* con_outputWindowColor = nullptr;
	game::dvar_t* con_inputDvarMatchColor = nullptr;
	game::dvar_t* con_inputDvarValueColor = nullptr;
	game::dvar_t* con_inputDvarInactiveValueColor = nullptr;
	game::dvar_t* con_inputCmdMatchColor = nullptr;

	game::dvar_t* g_playerEjection = nullptr;
	game::dvar_t* g_playerCollision = nullptr;

	game::dvar_t* bg_surfacePenetration = nullptr;

	game::dvar_t* pm_bouncing = nullptr;
	game::dvar_t* g_dump_scripts = nullptr;
	game::dvar_t* g_gravity = nullptr;
	game::dvar_t* g_speed = nullptr;
	game::dvar_t* g_elevators = nullptr;
	game::dvar_t* g_log = nullptr;

	game::dvar_t* jump_height = nullptr;
	game::dvar_t* jump_ladderPushVel = nullptr;

	game::dvar_t* player_sustainAmmo = nullptr;

	game::dvar_t* r_fullbright = nullptr;

	game::dvar_t* cg_legacyCrashHandling = nullptr;

	game::dvar_t* sv_cheats = nullptr;

	std::string dvar_get_vector_domain(const int components, const game::dvar_limits& domain)
	{
		if (domain.vector.min == -FLT_MAX)
		{
			if (domain.vector.max == FLT_MAX)
			{
				return utils::string::va("Domain is any %iD vector", components);
			}
			else
			{
				return utils::string::va("Domain is any %iD vector with components %g or smaller", components,
				                         domain.vector.max);
			}
		}
		else if (domain.vector.max == FLT_MAX)
		{
			return utils::string::va("Domain is any %iD vector with components %g or bigger", components,
			                         domain.vector.min);
		}
		else
		{
			return utils::string::va("Domain is any %iD vector with components from %g to %g", components,
			                         domain.vector.min, domain.vector.max);
		}
	}

	std::string dvar_get_domain(const game::dvar_type type, const game::dvar_limits& domain)
	{
		std::string str;

		switch (type)
		{
		case game::dvar_type::boolean:
			return "Domain is 0 or 1"s;

		case game::dvar_type::value:
			if (domain.value.min == -FLT_MAX)
			{
				if (domain.value.max == FLT_MAX)
				{
					return "Domain is any number"s;
				}
				else
				{
					return utils::string::va("Domain is any number %g or smaller", domain.value.max);
				}
			}
			else if (domain.value.max == FLT_MAX)
			{
				return utils::string::va("Domain is any number %g or bigger", domain.value.min);
			}
			else
			{
				return utils::string::va("Domain is any number from %g to %g", domain.value.min, domain.value.max);
			}

		case game::dvar_type::vec2:
			return dvar_get_vector_domain(2, domain);

		case game::dvar_type::rgb:
		case game::dvar_type::vec3:
			return dvar_get_vector_domain(3, domain);

		case game::dvar_type::vec4:
			return dvar_get_vector_domain(4, domain);

		case game::dvar_type::integer:
			if (domain.enumeration.stringCount == INT_MIN)
			{
				if (domain.integer.max == INT_MAX)
				{
					return "Domain is any integer"s;
				}
				else
				{
					return utils::string::va("Domain is any integer %i or smaller", domain.integer.max);
				}
			}
			else if (domain.integer.max == INT_MAX)
			{
				return utils::string::va("Domain is any integer %i or bigger", domain.integer.min);
			}
			else
			{
				return utils::string::va("Domain is any integer from %i to %i", domain.integer.min, domain.integer.max);
			}

		case game::dvar_type::color:
			return "Domain is any 4-component color, in RGBA format"s;

		case game::dvar_type::enumeration:
			str = "Domain is one of the following:"s;

			for (auto string_index = 0; string_index < domain.enumeration.stringCount; ++string_index)
			{
				str += utils::string::va("\n  %2i: %s", string_index, domain.enumeration.strings[string_index]);
			}

			return str;

		case game::dvar_type::string:
			return "Domain is any text"s;

		default:
			return utils::string::va("unhandled dvar type '%i'", type);
		}
	}
}
