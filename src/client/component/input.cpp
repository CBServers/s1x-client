#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "game/game.hpp"

#include "game_console.hpp"
#include "server_list.hpp"

#include <utils/hook.hpp>

namespace input
{
	namespace
	{
		utils::hook::detour cl_char_event_hook;
		utils::hook::detour cl_key_event_hook;

		void cl_char_event_stub(const int local_client_num, const int key)
		{
			if (!game_console::console_char_event(local_client_num, key))
			{
				return;
			}

			cl_char_event_hook.invoke<void>(local_client_num, key);
		}

		void cl_key_event_stub(const int local_client_num, const int key, const signed int down, const int arg4)
		{
			if (!game_console::console_key_event(local_client_num, key, down))
			{
				return;
			}

			if (game::environment::is_mp() && !server_list::sl_key_event(key, down))
			{
				return;
			}

			cl_key_event_hook.invoke<void>(local_client_num, key, down, arg4);
		}
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			if (game::environment::is_dedi())
			{
				return;
			}

			cl_char_event_hook.create(SELECT_VALUE(0x14013E7B0, 0x140206690), cl_char_event_stub);
			cl_key_event_hook.create(SELECT_VALUE(0x14013EB20, 0x140206900), cl_key_event_stub);
		}
	};
}

REGISTER_COMPONENT(input::component)
