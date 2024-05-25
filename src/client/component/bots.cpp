#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "game/game.hpp"
#include "game/scripting/execution.hpp"

#include "command.hpp"
#include "console.hpp"
#include "filesystem.hpp"
#include "network.hpp"
#include "party.hpp"
#include "scheduler.hpp"
#include "server_list.hpp"

#include <utils/hook.hpp>
#include <utils/string.hpp>

namespace bots
{
	namespace
	{
		constexpr std::size_t MAX_NAME_LENGTH = 16;

		bool can_add()
		{
			if (party::get_client_count() < *game::mp::svs_numclients)
			{
				return true;
			}

			return false;
		}

		void bot_team_join(const unsigned int entity_num)
		{
			const game::scr_entref_t entref{static_cast<uint16_t>(entity_num), 0};
			scheduler::once([entref]()
			{
				scripting::notify(entref, "luinotifyserver", {"team_select", 2});
				scheduler::once([entref]()
				{
					auto* _class = utils::string::va("class%d", std::rand() % 5);
					scripting::notify(entref, "luinotifyserver", {"class_select", _class});
				}, scheduler::pipeline::server, 2s);
			}, scheduler::pipeline::server, 2s);
		}

		void spawn_bot(const int entity_num)
		{
			game::SV_SpawnTestClient(&game::mp::g_entities[entity_num]);
			if (game::Com_GetCurrentCoDPlayMode() == game::CODPLAYMODE_CORE)
			{
				bot_team_join(entity_num);
			}
		}

		void add_bot()
		{
			if (!can_add())
			{
				return;
			}

			// SV_BotGetRandomName
			const auto* const bot_name = game::SV_BotGetRandomName();
			const auto* bot_ent = game::SV_AddBot(bot_name);
			if (bot_ent)
			{
				spawn_bot(bot_ent->s.number);
			}
			else if (can_add()) // workaround since first bot won't ever spawn
			{
				add_bot();
			}
		}

		utils::hook::detour get_bot_name_hook;
		volatile bool bot_names_received = false;
		std::vector<std::string> bot_names;

		bool should_use_remote_bot_names()
		{
			return !filesystem::exists("bots.txt");
		}

		void parse_bot_names_from_file()
		{
			std::string data;
			filesystem::read_file("bots.txt", &data);
			if (data.empty())
			{
				return;
			}

			auto name_list = utils::string::split(data, '\n');
			for (auto& entry : name_list)
			{
				// Take into account CR line endings
				entry = utils::string::replace(entry, "\r", "");

				if (entry.empty())
				{
					continue;
				}

				entry = entry.substr(0, MAX_NAME_LENGTH - 1);
				bot_names.emplace_back(entry);
			}
		}

		const char* get_random_bot_name()
		{
			if (!bot_names_received && bot_names.empty())
			{
				// last attempt to use custom names if they can be found
				parse_bot_names_from_file();
			}

			if (bot_names.empty())
			{
				return get_bot_name_hook.invoke<const char*>();
			}

			const auto index = std::rand() % bot_names.size();
			const auto& name = bot_names.at(index);

			return utils::string::va("%.*s", static_cast<int>(name.size()), name.data());
		}

		void update_bot_names()
		{
			bot_names_received = false;

			game::netadr_s master{};
			if (server_list::get_master_server(master))
			{
				console::info("Getting bots...\n");
				network::send(master, "getbots");
			}
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

			get_bot_name_hook.create(game::SV_BotGetRandomName, get_random_bot_name);

			command::add("spawnBot", [](const command::params& params)
			{
				if (!game::SV_Loaded() || game::VirtualLobby_Loaded()) return;

				auto num_bots = 1;
				if (params.size() == 2)
				{
					num_bots = std::atoi(params.get(1));
				}

				num_bots = std::min(num_bots, *game::mp::svs_numclients);

				console::info("Spawning %i %s\n", num_bots, (num_bots == 1 ? "bot" : "bots"));

				for (auto i = 0; i < num_bots; ++i)
				{
					scheduler::once(add_bot, scheduler::pipeline::server, 100ms * i);
				}
			});

			if (should_use_remote_bot_names())
			{
				scheduler::on_game_initialized([]() -> void
				{
					update_bot_names();
					scheduler::loop(update_bot_names, scheduler::main, 1h);
				}, scheduler::main);
			}
			else
			{
				parse_bot_names_from_file();
			}

			network::on("getbotsResponse", [](const game::netadr_s& target, const std::string_view& data)
			{
				game::netadr_s master{};
				if (server_list::get_master_server(master) && !bot_names_received && target == master)
				{
					const std::string received_data{ data };
					bot_names = utils::string::split(received_data, '\n');
					console::info("Got %zu names from the master server\n", bot_names.size());
					bot_names_received = true;
				}
			});
		}
	};
}

REGISTER_COMPONENT(bots::component)
