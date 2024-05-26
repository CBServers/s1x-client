#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "game/game.hpp"

#include "game/scripting/entity.hpp"
#include "game/scripting/execution.hpp"

#include "command.hpp"
#include "scheduler.hpp"
#include "notifies.hpp"
#include "scripting.hpp"
#include "game_log.hpp"

#include <utils/hook.hpp>

namespace notifies
{
	bool hook_enabled = true;

	namespace
	{
		struct gsc_hook
		{
			const char* target_pos{};
		};

		std::unordered_map<const char*, gsc_hook> vm_execute_hooks;

		char empty_function[2] = {0x32, 0x34}; // CHECK_CLEAR_PARAMS, END
		const char* target_function = nullptr;

		

		void client_command_stub(const int client_num)
		{
			if (game::mp::g_entities[client_num].client == nullptr)
			{
				return;
			}

			command::params_sv params;

			if (params[0] == "say"s || params[0] == "say_team"s)
			{
				const std::string cmd(params[0]); //ensure params[0] is the same when used later
				std::string message(params.join(1));

				auto msg_index = 0;
				if (message[msg_index] == '\x1F')
				{
					msg_index = 1;
				}

				auto hidden = false;
				if (message[msg_index] == '/')
				{
					hidden = true;

					if (msg_index == 1)
					{
						// Overwrite / with \x1F only if present
						message[msg_index] = message[msg_index - 1];
					}
					// Skip over the first character
					message.erase(message.begin());
				}

				scheduler::once([cmd, message, msg_index, hidden, client_num]
				{
					const scripting::entity level{ *game::levelEntityId };
					const auto player = scripting::call("getentbynum", { client_num }).as<scripting::entity>();
					// Remove \x1F before sending the notify only if present
					const auto notify_msg = msg_index ? message.substr(1) : message;

					scripting::notify(level, cmd, {player, notify_msg, hidden});
					scripting::notify(player, cmd, {notify_msg, hidden});

					game_log::g_log_printf("%s;%s;%i;%s;%s\n",
						cmd.data(),
						player.call("getguid").as<const char*>(),
						client_num,
						player.get("name").as<const char*>(),
						message.data()
					);

				}, scheduler::pipeline::server);
			}

			// ClientCommand
			utils::hook::invoke<void>(0x1402E98F0, client_num);
		}

		unsigned int local_id_to_entity(unsigned int local_id)
		{
			const auto variable = game::scr_VarGlob->objectVariableValue[local_id];
			return variable.u.f.next;
		}

		bool execute_vm_hook(const char* pos)
		{
			if (!vm_execute_hooks.contains(pos))
			{
				hook_enabled = true;
				return false;
			}

			if (!hook_enabled && pos > reinterpret_cast<char*>(vm_execute_hooks.size()))
			{
				hook_enabled = true;
				return false;
			}

			const auto hook = vm_execute_hooks[pos];
			target_function = hook.target_pos;

			return true;
		}

		void vm_execute_stub(utils::hook::assembler& a)
		{
			const auto replace = a.newLabel();
			const auto end = a.newLabel();

			a.pushad64();

			a.mov(rcx, r14);
			a.call_aligned(execute_vm_hook);

			a.cmp(al, 0);
			a.jne(replace);

			a.popad64();
			a.jmp(end);

			a.bind(end);

			a.movzx(r15d, byte_ptr(r14));
			a.inc(r14);
			a.lea(eax, dword_ptr(r15, -0x17));
			a.mov(dword_ptr(rbp, 0x68), r15d);

			a.jmp(0x1403FA143);

			a.bind(replace);

			a.popad64();
			a.mov(rax, qword_ptr(reinterpret_cast<std::int64_t>(&target_function)));
			a.mov(r14, rax);
			a.jmp(end);
		}
	}

	void clear_callbacks()
	{
		vm_execute_hooks.clear();
	}

	void enable_vm_execute_hook()
	{
		hook_enabled = true;
	}

	void disable_vm_execute_hook()
	{
		hook_enabled = false;
	}

	void set_gsc_hook(const char* source, const char* target)
	{
		gsc_hook hook;
		hook.target_pos = target;
		vm_execute_hooks[source] = hook;
	}

	void clear_hook(const char* pos)
	{
		vm_execute_hooks.erase(pos);
	}

	std::size_t get_hook_count()
	{
		return vm_execute_hooks.size();
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

			utils::hook::call(0x14043A9AD, client_command_stub);

			utils::hook::jump(0x1403FA134, utils::hook::assemble(vm_execute_stub), true);

			scripting::on_shutdown([](bool free_scripts)
			{
				if (free_scripts)
				{
					vm_execute_hooks.clear();
				}
			});
		}
	};
}

REGISTER_COMPONENT(notifies::component)
