#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "scheduler.hpp"
#include "fps.hpp"
#include "localized_strings.hpp"

#include "game/game.hpp"
#include "game/dvars.hpp"

#include <utils/hook.hpp>
#include <utils/string.hpp>

namespace fps
{
	namespace
	{
		const game::dvar_t* cg_drawFPS;
		const game::dvar_t* cg_drawPing;

		float fps_color_good[4] = {0.6f, 1.0f, 0.0f, 1.0f};
		float fps_color_ok[4] = {1.0f, 0.7f, 0.3f, 1.0f};
		float fps_color_bad[4] = {1.0f, 0.3f, 0.3f, 1.0f};

		//float origin_color[4] = { 1.0f, 0.67f, 0.13f, 1.0f };
		float ping_color[4] = {1.0f, 1.0f, 1.0f, 0.65f};

		struct cg_perf_data
		{
			std::chrono::time_point<std::chrono::steady_clock> perf_start;
			std::int32_t current_ms{};
			std::int32_t previous_ms{};
			std::int32_t frame_ms{};
			std::int32_t history[32]{};
			std::int32_t count{};
			std::int32_t index{};
			std::int32_t instant{};
			std::int32_t total{};
			float average{};
			float variance{};
			std::int32_t min{};
			std::int32_t max{};
		};

		cg_perf_data cg_perf = cg_perf_data();

		void perf_calc_fps(cg_perf_data* data, const std::int32_t value)
		{
			data->history[data->index % 32] = value;
			data->instant = value;
			data->min = std::numeric_limits<int>::max();
			data->max = 0;
			data->average = 0.0f;
			data->variance = 0.0f;
			data->total = 0;

			for (auto i = 0; i < data->count; ++i)
			{
				const std::int32_t idx = (data->index - i) % 32;

				if (idx < 0)
				{
					break;
				}

				data->total += data->history[idx];

				if (data->min > data->history[idx])
				{
					data->min = data->history[idx];
				}

				if (data->max < data->history[idx])
				{
					data->max = data->history[idx];
				}
			}

			data->average = static_cast<float>(data->total) / static_cast<float>(data->count);
			++data->index;
		}

		void perf_update()
		{
			cg_perf.count = 32;

			cg_perf.current_ms = static_cast<std::int32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::high_resolution_clock::now() - cg_perf.perf_start).count());
			cg_perf.frame_ms = cg_perf.current_ms - cg_perf.previous_ms;
			cg_perf.previous_ms = cg_perf.current_ms;

			perf_calc_fps(&cg_perf, cg_perf.frame_ms);

			utils::hook::invoke<void>(SELECT_VALUE(0x1404F6A90, 0x14062C540));
		}

		void cg_draw_fps()
		{
			if (cg_drawFPS && cg_drawFPS->current.integer > 0)
			{
				const auto fps = get_fps();

				auto* font = game::R_RegisterFont("fonts/consolefont");
				if (!font) return;

				const auto* const fps_string = utils::string::va("%i", fps);

				const auto scale = 1.0f;

				const auto x = (game::ScrPlace_GetViewPlacement()->realViewportSize[0] - 10.0f) - game::R_TextWidth(
					fps_string, std::numeric_limits<int>::max(), font) * scale;

				const auto y = font->pixelHeight * 1.2f;

				const auto fps_color = fps >= 60 ? fps_color_good : (fps >= 30 ? fps_color_ok : fps_color_bad);
				game::R_AddCmdDrawText(fps_string, std::numeric_limits<int>::max(), font, x, y, scale, scale, 0.0f, fps_color, 6);
			}
		}

		void cg_draw_ping()
		{
			if (cg_drawPing->current.integer > 0 && game::CL_IsCgameInitialized())
			{
				auto* font = game::R_RegisterFont("fonts/consolefont");
				if (!font) return;

				auto* const ping_string = utils::string::va("Ping: %i", *game::mp::ping);

				const auto scale = 1.0f;

				const auto x = (game::ScrPlace_GetViewPlacement()->realViewportSize[0] - 375.0f) - game::R_TextWidth(
					ping_string, std::numeric_limits<int>::max(), font) * scale;

				const auto y = font->pixelHeight * 1.2f;

				game::R_AddCmdDrawText(ping_string, std::numeric_limits<int>::max(), font, x, y, scale, scale, 0.0f, ping_color, 6);
			}
		}

		const game::dvar_t* cg_draw_fps_register_stub(const char* dvar_name, const char** value_list, const int default_index, unsigned int /*flags*/, const char* description)
		{
			cg_drawFPS = game::Dvar_RegisterEnum(dvar_name, value_list, default_index, game::DVAR_FLAG_SAVED, description);
			return cg_drawFPS;
		}
	}

	int get_fps()
	{
		return static_cast<std::int32_t>(static_cast<float>(1000.0f /
			static_cast<float>(cg_perf.average)) + 9.313225746154785e-10);
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

			// fps setup
			cg_perf.perf_start = std::chrono::high_resolution_clock::now();
			utils::hook::call(SELECT_VALUE(0x140144D41, 0x140213B27), &perf_update);

			// change cg_drawfps flags to saved
			utils::hook::call(SELECT_VALUE(0x1400EF951, 0x1401A4B8E), &cg_draw_fps_register_stub);

			// fix ping value
			utils::hook::nop(0x140213031, 2);

			scheduler::loop(cg_draw_fps, scheduler::pipeline::renderer);
			if (game::environment::is_mp())
			{
				cg_drawPing = game::Dvar_RegisterInt("cg_drawPing", 0, 0, 1, game::DVAR_FLAG_SAVED, "Draw ping");
				scheduler::loop(cg_draw_ping, scheduler::pipeline::renderer);
			}

			game::Dvar_RegisterBool("cg_infobar_ping", false, game::DVAR_FLAG_SAVED, "Show server latency");
			game::Dvar_RegisterBool("cg_infobar_fps", false, game::DVAR_FLAG_SAVED, "Show FPS counter");
		}
	};
}

REGISTER_COMPONENT(fps::component)
