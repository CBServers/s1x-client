#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "game/game.hpp"

#include "console.hpp"

#include <utils/hook.hpp>

namespace security
{
	namespace
	{
		void set_cached_playerdata_stub(const int localclient, const int index1, const int index2)
		{
			if (index1 >= 0 && index1 < 18 && index2 >= 0 && index2 < 42)
			{
				reinterpret_cast<void(*)(int, int, int)>(0x140536A60)(localclient, index1, index2);
			}
		}

		void remap_cached_entities(game::mp::cachedSnapshot_t& snapshot)
		{
			static bool printed = false;
			if(snapshot.num_clients > 1200 && !printed)
			{
				printed = true;
				console::info("Too many entities (%d)... remapping!\n", snapshot.num_clients);
			}
			
			snapshot.num_clients = std::min(snapshot.num_clients, 1200);
		}

		void remap_cached_entities_stub(utils::hook::assembler& a)
		{
			a.pushad64();

			a.mov(rcx, rbx);
			a.call_aligned(remap_cached_entities);

			a.popad64();
			a.jmp(0x14044DE51);
		}

		void sv_execute_client_message_stub(game::mp::client_t* client, game::msg_t* msg)
		{
			if ((client->reliableSequence - client->reliableAcknowledge) < 0)
			{
				client->reliableAcknowledge = client->reliableSequence;
				console::info("Negative reliableAcknowledge from %s - cl->reliableSequence is %i, reliableAcknowledge is %i\n",
				              client->name, client->reliableSequence, client->reliableAcknowledge);
				return;
			}

			utils::hook::invoke<void>(0x14043AA90, client, msg);
		}
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			if (game::environment::is_sp()) return;

			// Patch vulnerability in PlayerCards_SetCachedPlayerData
			utils::hook::call(0x1401BB909, set_cached_playerdata_stub);

			// Patch entity overflow
			utils::hook::jump(0x14044DE3A, assemble(remap_cached_entities_stub), true);

			// It is possible to make the server hang if left unchecked
			utils::hook::call(0x140443051, sv_execute_client_message_stub);
		}
	};
}

REGISTER_COMPONENT(security::component)
