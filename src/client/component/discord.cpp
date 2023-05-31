#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "game/game.hpp"

#include "console.hpp"
#include "network.hpp"
#include "party.hpp"
#include "scheduler.hpp"

#include <utils/string.hpp>
#include <utils/cryptography.hpp>

#include <discord_rpc.h>

namespace discord
{
	namespace
	{
		DiscordRichPresence discord_presence;

		void join_game(const char* join_secret)
		{
			game::Cbuf_AddText(0, utils::string::va("connect %s\n", join_secret));
		}

		void join_request(const DiscordUser* request)
		{
#ifdef _DEBUG
			console::info("Discord: Join request from %s (%s)\n", request->username, request->userId);
#endif
			Discord_Respond(request->userId, DISCORD_REPLY_IGNORE);
		}

		void update_discord()
		{
			Discord_RunCallbacks();

			auto* dvar = game::Dvar_FindVar("virtualLobbyActive");
			if (!game::CL_IsCgameInitialized() || (dvar && dvar->current.enabled == 1))
			{
				discord_presence.details = game::environment::is_sp() ? "Singleplayer" : "Multiplayer";

				dvar = game::Dvar_FindVar("virtualLobbyInFiringRange");
				if (dvar && dvar->current.enabled == 1)
				{
					discord_presence.state = "Firing Range";
				}
				else
				{
					discord_presence.state = "Main Menu";
				}

				discord_presence.partySize = 0;
				discord_presence.partyMax = 0;
				discord_presence.startTimestamp = 0;

				discord_presence.largeImageKey = game::environment::is_sp() ? "menu_singleplayer" : "menu_multiplayer";
			}
			else
			{
				if (game::environment::is_sp()) return;

				const auto* gametype = game::UI_GetGameTypeDisplayName(game::Dvar_FindVar("ui_gametype")->current.string);
				const auto* map = game::UI_GetMapDisplayName(game::Dvar_FindVar("ui_mapname")->current.string);

				discord_presence.details = utils::string::va("%s on %s", gametype, map);

				// get server host name
				auto* const host_name = reinterpret_cast<char*>(0x141646CC4);
				utils::string::strip(host_name, host_name, std::strlen(host_name) + 1);

				// get number of clients in game
				auto clients = reinterpret_cast<int*>(0x1414CC290);
				int clientsNum = *clients;
				discord_presence.partySize = clientsNum;

				if (game::Dvar_FindVar("name") && !strcmp(host_name, game::Dvar_FindVar("name")->current.string)) // host_name == name, most likely private match
				{
					discord_presence.state = "Private Match";
					discord_presence.partyMax = game::Dvar_FindVar("sv_maxclients")->current.integer;
				}
				else
				{
					discord_presence.state = host_name;
					discord_presence.partyMax = party::server_client_count();

					std::hash<game::netadr_s> hash_fn;
					static const auto nonce = utils::cryptography::random::get_integer();

					const auto& address = party::get_target();
					discord_presence.partyId = utils::string::va("%zu", hash_fn(address) ^ nonce);
					discord_presence.joinSecret = network::net_adr_to_string(address);
				}

				if (!discord_presence.startTimestamp)
				{
					discord_presence.startTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
						std::chrono::system_clock::now().time_since_epoch()).count();
				}

				discord_presence.largeImageKey = game::Dvar_FindVar("ui_mapname")->current.string;
				discord_presence.largeImageText = game::UI_GetMapDisplayName(game::Dvar_FindVar("ui_mapname")->current.string);
			}

			Discord_UpdatePresence(&discord_presence);
		}
	}

	class component final : public component_interface
	{
	public:
		void post_load() override
		{
			if (game::environment::is_dedi())
			{
				return;
			}

			DiscordEventHandlers handlers;
			ZeroMemory(&handlers, sizeof(handlers));
			handlers.ready = ready;
			handlers.errored = errored;
			handlers.disconnected = errored;
			handlers.joinGame = join_game;
			handlers.spectateGame = nullptr;
			handlers.joinRequest = join_request;

			Discord_Initialize("823223724013912124", &handlers, 1, nullptr);

			scheduler::once([]
			{
				scheduler::once(update_discord, scheduler::pipeline::async);
				scheduler::loop(update_discord, scheduler::pipeline::async, 15s);
			}, scheduler::pipeline::main);

			initialized_ = true;
		}

		void pre_destroy() override
		{
			if (!initialized_)
			{
				return;
			}

			Discord_Shutdown();
		}

	private:
		bool initialized_ = false;

		static void ready(const DiscordUser* /*request*/)
		{
			ZeroMemory(&discord_presence, sizeof(discord_presence));

			discord_presence.instance = 1;

			console::info("Discord: Ready\n");

			Discord_UpdatePresence(&discord_presence);
		}

		static void errored(const int error_code, const char* message)
		{
			console::error("Discord: Error (%i): %s\n", error_code, message);
		}
	};
}

#ifndef DEV_BUILD
REGISTER_COMPONENT(discord::component)
#endif
