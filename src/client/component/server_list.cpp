#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "game/game.hpp"
#include "game/dvars.hpp"

#include "server_list.hpp"
#include "localized_strings.hpp"
#include "network.hpp"
#include "scheduler.hpp"
#include "party.hpp"

#include <utils/cryptography.hpp>
#include <utils/string.hpp>
#include <utils/hook.hpp>

namespace server_list
{
	namespace
	{
		const int server_limit = 14;

		struct server_info
		{
			// gotta add more to this
			int clients;
			int max_clients;
			int bots;
			int ping;
			std::string host_name;
			std::string map_name;
			std::string game_type;
			game::CodPlayMode play_mode;
			char in_game;
			game::netadr_s address;
		};

		struct
		{
			game::netadr_s address{};
			volatile bool requesting = false;
			std::unordered_map<game::netadr_s, int> queued_servers{};
		} master_state;

		std::mutex mutex;
		std::vector<server_info> servers;

		size_t server_list_page = 0;
		volatile bool update_server_list = false;
		std::chrono::high_resolution_clock::time_point last_scroll{};

		game::dvar_t* master_server_ip;
		game::dvar_t* master_server_port;

		size_t get_page_count()
		{
			const auto count = servers.size() / server_limit;
			return count + (servers.size() % server_limit > 0);
		}

		size_t get_page_base_index()
		{
			return server_list_page * server_limit;
		}

		void refresh_server_list()
		{
			{
				std::lock_guard<std::mutex> _(mutex);
				servers.clear();
				master_state.queued_servers.clear();
				server_list_page = 0;
			}

			party::reset_connect_state();

			if (get_master_server(master_state.address))
			{
				master_state.requesting = true;
				network::send(master_state.address, "getservers", utils::string::va("S1 %i full empty", PROTOCOL));
			}
		}

		void join_server(int, int, const int index)
		{
			std::lock_guard<std::mutex> _(mutex);

			const auto i = static_cast<size_t>(index) + get_page_base_index();
			if (i < servers.size())
			{
				static auto last_index = ~0ull;
				if (last_index != i)
				{
					last_index = i;
				}
				else
				{
					printf("Connecting to (%d - %zu): %s\n", index, i, servers[i].host_name.data());
					party::connect(servers[i].address);
				}
			}
		}

		void trigger_refresh()
		{
			update_server_list = true;
		}

		int ui_feeder_count()
		{
			std::lock_guard<std::mutex> _(mutex);
			if (update_server_list)
			{
				update_server_list = false;
				return 0;
			}
			const auto count = static_cast<int>(servers.size());
			const auto index = get_page_base_index();
			const auto diff = count - index;
			return diff > server_limit ? server_limit : static_cast<int>(diff);
		}

		const char* ui_feeder_item_text(int /*localClientNum*/, void* /*a2*/, void* /*a3*/, const int index,
		                                const int column)
		{
			std::lock_guard<std::mutex> _(mutex);

			const auto i = get_page_base_index() + index;

			if (i >= servers.size())
			{
				return "";
			}

			if (column == 0)
			{
				return servers[i].host_name.empty() ? "" : utils::string::va("%s", servers[i].host_name.data());
			}

			if (column == 1)
			{
				return servers[i].map_name.empty() ? "" : utils::string::va("%s", servers[i].map_name.data());
			}

			if (column == 2)
			{
				return servers[i].game_type.empty() ? "" : utils::string::va("%s", servers[i].game_type.data());
			}

			if (column == 3)
			{
				return utils::string::va("%d/%d [%d]", servers[i].clients, servers[i].max_clients,
					servers[i].bots);
			}

			if (column == 4)
			{
				return servers[i].ping ? utils::string::va("%d", servers[i].ping) : "";
			}

			return "";
		}

		void sort_serverlist()
		{
			std::ranges::stable_sort(servers, [](const server_info& a, const server_info& b)
			{
				if (a.clients == b.clients)
				{
					return a.ping < b.ping;
				}

				return a.clients > b.clients;
			});
		}

		void insert_server(server_info&& server)
		{
			std::lock_guard<std::mutex> _(mutex);
			servers.emplace_back(std::move(server));
			sort_serverlist();
			trigger_refresh();
		}

		void do_frame_work()
		{
			auto& queue = master_state.queued_servers;
			if (queue.empty())
			{
				return;
			}

			std::lock_guard<std::mutex> _(mutex);

			size_t queried_servers = 0;
			const size_t query_limit = 3;

			for (auto i = queue.begin(); i != queue.end();)
			{
				if (i->second)
				{
					const auto now = game::Sys_Milliseconds();
					if (now - i->second > 10'000)
					{
						i = queue.erase(i);
						continue;
					}
				}
				else if (queried_servers++ < query_limit)
				{
					i->second = game::Sys_Milliseconds();
					network::send(i->first, "getInfo", utils::cryptography::random::get_challenge());
				}

				++i;
			}
		}

		bool is_server_list_open()
		{
			return game::Menu_IsMenuOpenAndVisible(0, "menu_systemlink_join");
		}

		bool is_scrolling_disabled()
		{
			return update_server_list || (std::chrono::high_resolution_clock::now() - last_scroll) < 500ms;
		}

		bool scroll_down()
		{
			if (!is_server_list_open())
			{
				return false;
			}

			if (!is_scrolling_disabled() && server_list_page + 1 < get_page_count())
			{
				last_scroll = std::chrono::high_resolution_clock::now();
				++server_list_page;
				trigger_refresh();
			}

			return true;
		}

		bool scroll_up()
		{
			if (!is_server_list_open())
			{
				return false;
			}

			if (!is_scrolling_disabled() && server_list_page > 0)
			{
				last_scroll = std::chrono::high_resolution_clock::now();
				--server_list_page;
				trigger_refresh();
			}

			return true;
		}

		void resize_host_name(std::string& name)
		{
			name = utils::string::split(name, '\n').front();

			game::Font_s* font;
			if (game::Com_GetCurrentCoDPlayMode() == game::CODPLAYMODE_ZOMBIES)
			{
				font = game::R_RegisterFont("fonts/zmBodyFont");
			}
			else
			{
				font = game::R_RegisterFont("fonts/bodyFont");
			}
			auto text_size = game::UI_TextWidth(name.data(), 32, font, 1.0f);

			while (text_size > 450)
			{
				text_size = game::UI_TextWidth(name.data(), 32, font, 1.0f);
				name.pop_back();
			}
		}

		void lui_open_menu_stub(int /*controllerIndex*/, const char* /*menu*/, int /*a3*/, int /*a4*/,
		                        unsigned int /*a5*/)
		{
			refresh_server_list();
			game::Cmd_ExecuteSingleCommand(0, 0, "lui_open menu_systemlink_join\n");
		}
	}

	bool sl_key_event(const int key, const int down)
	{
		if (down)
		{
			if (key == game::keyNum_t::K_MWHEELUP)
			{
				return !scroll_up();
			}

			if (key == game::keyNum_t::K_MWHEELDOWN)
			{
				return !scroll_down();
			}
		}

		return true;
	}

	bool get_master_server(game::netadr_s& address)
	{
		return game::NET_StringToAdr(utils::string::va("%s:%s",
			master_server_ip->current.string, master_server_port->current.string), &address);
	}

	void handle_info_response(const game::netadr_s& address, const utils::info_string& info)
	{
		// Don't show servers that aren't dedicated!
		const auto dedicated = info.get("dedicated");
		if (dedicated != "1"s)
		{
			return;
		}

		// Don't show servers that aren't running!
		const auto sv_running = info.get("sv_running");
		if (sv_running != "1"s)
		{
			return;
		}

		// Only handle servers of the same playmode!
		const auto playmode = static_cast<game::CodPlayMode>(std::atoi(info.get("playmode").data()));
		if (game::Com_GetCurrentCoDPlayMode() != playmode)
		{
			return;
		}

		int start_time{};
		const auto now = game::Sys_Milliseconds();

		{
			std::lock_guard<std::mutex> _(mutex);
			const auto entry = master_state.queued_servers.find(address);

			if (entry == master_state.queued_servers.end() || !entry->second)
			{
				return;
			}

			start_time = entry->second;
			master_state.queued_servers.erase(entry);
		}

		server_info server{};
		server.address = address;
		server.host_name = info.get("hostname");
		server.map_name = game::UI_GetMapDisplayName(info.get("mapname").data());
		server.game_type = game::UI_GetGameTypeDisplayName(info.get("gametype").data());
		server.play_mode = playmode;
		server.clients = std::atoi(info.get("clients").data());
		server.max_clients = std::atoi(info.get("sv_maxclients").data());
		server.bots = std::atoi(info.get("bots").data());
		server.ping = std::min(now - start_time, 999);

		server.in_game = 1;

		resize_host_name(server.host_name);

		insert_server(std::move(server));
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			if (!game::environment::is_sp())
			{
				scheduler::once([]()
				{
					// add dvars to change destination master server ip/port
					master_server_ip = game::Dvar_RegisterString("masterServerIP", "master.cbservers.xyz", game::DVAR_FLAG_NONE,
						"IP of the destination master server to connect to");
					master_server_port = game::Dvar_RegisterString("masterServerPort", "20810", game::DVAR_FLAG_NONE,
							"Port of the destination master server to connect to");
				}, scheduler::pipeline::main);
			}

			if (!game::environment::is_mp())
			{
				return;
			}

			// replace UI_RunMenuScript call in LUI_CoD_LuaCall_RefreshServerList to our refresh_servers
			utils::hook::call(0x1400F5AA1, &refresh_server_list);
			utils::hook::call(0x1400F5F26, &join_server);
			utils::hook::nop(0x1400F5F45, 5);

			// do feeder stuff
			utils::hook::call(0x1400F5B55, &ui_feeder_count);
			utils::hook::call(0x1400F5D35, &ui_feeder_item_text);

			scheduler::loop(do_frame_work, scheduler::pipeline::main);

			network::on("getServersResponse", [](const game::netadr_s& target, const std::string_view& data)
			{
				{
					std::lock_guard<std::mutex> _(mutex);
					if (!master_state.requesting || master_state.address != target)
					{
						return;
					}

					master_state.requesting = false;

					std::optional<size_t> start{};
					for (size_t i = 0; i + 6 < data.size(); ++i)
					{
						if (data[i + 6] == '\\')
						{
							start.emplace(i);
							break;
						}
					}

					if (!start.has_value())
					{
						return;
					}

					for (auto i = start.value(); i + 6 < data.size(); i += 7)
					{
						if (data[i + 6] != '\\')
						{
							break;
						}

						game::netadr_s address{};
						address.type = game::NA_IP;
						address.localNetID = game::NS_CLIENT1;
						memcpy(&address.ip[0], data.data() + i + 0, 4);
						memcpy(&address.port, data.data() + i + 4, 2);

						master_state.queued_servers[address] = 0;
					}
				}
			});
		}
	};
}

REGISTER_COMPONENT(server_list::component)
