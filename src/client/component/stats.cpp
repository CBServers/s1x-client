#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"

#include "command.hpp"
#include "console.hpp"

#include <utils/hook.hpp>

namespace stats
{
	namespace
	{
		const game::dvar_t* cg_unlock_all_items;
		const game::dvar_t* cg_unlock_all_loot;

		utils::hook::detour is_item_locked_hook1;
		utils::hook::detour is_item_locked_hook2;
		utils::hook::detour is_item_locked_hook3;

		int is_item_locked_stub1(void* a1, void* a2, void* a3)
		{
			if (cg_unlock_all_items->current.enabled)
			{
				return 0;
			}

			return is_item_locked_hook1.invoke<int>(a1, a2, a3);
		}

		int is_item_locked_stub2(void* a1, void* a2, void* a3, void* a4, void* a5)
		{
			if (cg_unlock_all_items->current.enabled)
			{
				return 0;
			}

			return is_item_locked_hook2.invoke<int>(a1, a2, a3, a4, a5);
		}

		int is_loot_locked_stub(void* a1)
		{
			if (cg_unlock_all_loot->current.enabled)
			{
				return 0;
			}

			return is_item_locked_hook3.invoke<int>(a1);
		}

		int is_item_locked()
		{
			return 0;
		}
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			if (game::environment::is_sp())
			{
				return;
			}

			if (game::environment::is_dedi())
			{
				// unlock all
				utils::hook::jump(0x1403BD790, is_item_locked); // LiveStorage_IsItemUnlockedFromTable_LocalClient
				utils::hook::jump(0x1403BD290, is_item_locked); // LiveStorage_IsItemUnlockedFromTable
				utils::hook::jump(0x1403BAF60, is_item_locked); // unlocks supply drop loot
			}
			else
			{
				// unlock all
				cg_unlock_all_items = game::Dvar_RegisterBool("cg_unlockall_items", false, game::DVAR_FLAG_SAVED, "Unlock items that are level-locked by the player's stats.");
				cg_unlock_all_loot = game::Dvar_RegisterBool("cg_unlockall_loot", false, game::DVAR_FLAG_SAVED, "Unlock supply drop loot.");
				game::Dvar_RegisterBool("cg_unlockall_classes", false, game::DVAR_FLAG_SAVED, "Unlock extra class slots.");

				is_item_locked_hook1.create(0x1403BD790, is_item_locked_stub1); // LiveStorage_IsItemUnlockedFromTable_LocalClient
				is_item_locked_hook2.create(0x1403BD290, is_item_locked_stub2); // LiveStorage_IsItemUnlockedFromTable
				is_item_locked_hook3.create(0x1403BAF60, is_loot_locked_stub);  // unlocks supply drop loot
			}

			command::add("setPlayerDataInt", [](const command::params& params)
			{
				if (params.size() < 2)
				{
					console::info("usage: setPlayerDataInt <data>, <value>\n");
					return;
				}

				// SL_FindString
				const auto lookup_string = game::SL_FindString(params.get(1));
				const auto value = atoi(params.get(2));

				// SetPlayerDataInt
				reinterpret_cast<void(*)(signed int, unsigned int, unsigned int, unsigned int)>(0x1403BF550)(
					0, lookup_string, value, 0);
			});

			command::add("getPlayerDataInt", [](const command::params& params)
			{
				if (params.size() < 2)
				{
					console::info("usage: getPlayerDataInt <data>\n");
					return;
				}

				// SL_FindString
				const auto lookup_string = game::SL_FindString(params.get(1));

				// GetPlayerDataInt
				const auto result = reinterpret_cast<int(*)(signed int, unsigned int, unsigned int)>(0x1403BE860)(
					0, lookup_string, 0);
				console::info("%d\n", result);
			});

			command::add("unlockstats", []()
			{
				command::execute("setPlayerDataInt prestige 30");
				command::execute("setPlayerDataInt experience 1002100");
			});
		}
	};
}

REGISTER_COMPONENT(stats::component)
