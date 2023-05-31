#include <std_include.hpp>

#include "engine.hpp"
#include "context.hpp"
#include "game/scripting/execution.hpp"

#include "component/notifies.hpp"
#include "component/game_module.hpp"

#include <utils/io.hpp>

namespace scripting::lua::engine
{
	namespace
	{
		auto& get_scripts()
		{
			static std::vector<std::unique_ptr<context>> scripts{};
			return scripts;
		}

		void load_scripts(const std::string& script_dir)
		{
			if (!utils::io::directory_exists(script_dir))
			{
				return;
			}

			const auto scripts = utils::io::list_files(script_dir);

			for (const auto& script : scripts)
			{
				if (std::filesystem::is_directory(script) && utils::io::file_exists(script + "/__init__.lua"))
				{
					get_scripts().push_back(std::make_unique<context>(script));
				}
			}
		}
	}

	void stop()
	{
		notifies::clear_callbacks();
		get_scripts().clear();
	}

	void start()
	{
		// No SP until there is a concept
		if (game::environment::is_sp())
		{
			return;
		}

		stop();
		load_scripts(game_module::get_host_module().get_folder() + "/data/lua-scripts/");
		load_scripts("s1x/lua-scripts/");
		load_scripts("data/lua-scripts/");
	}

	void notify(const event& e)
	{
		for (auto& script : get_scripts())
		{
			script->notify(e);
		}
	}

	void run_frame()
	{
		for (auto& script : get_scripts())
		{
			script->run_frame();
		}
	}
}
