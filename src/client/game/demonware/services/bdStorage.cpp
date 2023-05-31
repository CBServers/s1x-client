#include <std_include.hpp>
#include "game/game.hpp"

#include "../services.hpp"

#include <utils/nt.hpp>
#include <utils/io.hpp>
#include <utils/cryptography.hpp>

#include <component/motd.hpp>
#include <component/filesystem.hpp>

namespace demonware
{
	bdStorage::bdStorage() : service(10, "bdStorage")
	{
		this->register_task(6, &bdStorage::list_publisher_files);
		this->register_task(7, &bdStorage::get_publisher_file);
		this->register_task(10, &bdStorage::set_user_file);
		this->register_task(12, &bdStorage::get_user_file);
		this->register_task(13, &bdStorage::unk13);

		this->map_publisher_resource_variant(".*\\motd-.*\\.txt", motd::get_text);
		this->map_publisher_resource("ffotd-.*\\.ff", "dw/ffotd-1.22.1.ff", DW_FASTFILE);
		this->map_publisher_resource("playlists(_.+)?\\.aggr", "dw/playlists_tu22.aggr", DW_PLAYLISTS);
		this->map_publisher_resource("social_[Tt][Uu][0-9]+\\.cfg", "dw/social_tu22.cfg", DW_SOCIAL_CONFIG);
		this->map_publisher_resource("mm\\.cfg", "dw/mm.cfg", DW_MM_CONFIG);
		this->map_publisher_resource("entitlement_config\\.info", "dw/entitlement_config.info", DW_ENTITLEMENT_CONFIG);
		this->map_publisher_resource("lootConfig_[Tt][Uu][0-9]+\\.csv", "dw/lootConfig_tu22.csv", DW_LOOT_CONFIG);
		this->map_publisher_resource("winStoreConfig_[Tt][Uu][0-9]+\\.csv", "dw/winStoreConfig_tu22.csv", DW_STORE_CONFIG);
	}

	void bdStorage::map_publisher_resource(const std::string& expression, const std::string& path, const int id)
	{
		auto data = filesystem::exists(path)
			? filesystem::read_file(path)
			: utils::nt::load_resource(id);

		this->map_publisher_resource_variant(expression, std::move(data));
	}

	void bdStorage::map_publisher_resource_variant(const std::string& expression, resource_variant resource)
	{
		if (resource.valueless_by_exception())
		{
			throw std::runtime_error("Publisher resource variant is empty!");
		}

		this->publisher_resources_.emplace_back(std::regex{expression}, std::move(resource));
	}

	bool bdStorage::load_publisher_resource(const std::string& name, std::string& buffer) const
	{
		for (const auto& resource : this->publisher_resources_)
		{
			if (std::regex_match(name, resource.first))
			{
				if (std::holds_alternative<std::string>(resource.second))
				{
					buffer = std::get<std::string>(resource.second);
				}
				else
				{
					buffer = std::get<callback>(resource.second)();
				}

				return true;
			}
		}

#ifdef DEBUG
		printf("[DW]: [bdStorage]: missing publisher file: %s\n", name.data());
#endif

		return false;
	}

	void bdStorage::list_publisher_files(service_server* server, byte_buffer* buffer)
	{
		uint32_t date;
		uint16_t num_results, offset;
		std::string filename, data;

		buffer->read_uint32(&date);
		buffer->read_uint16(&num_results);
		buffer->read_uint16(&offset);
		buffer->read_string(&filename);

		auto reply = server->create_reply(this->task_id());

		if (this->load_publisher_resource(filename, data))
		{
			auto* info = new bdFileInfo;

			info->file_id = *reinterpret_cast<const uint64_t*>(utils::cryptography::sha1::compute(filename).data());
			info->filename = filename;
			info->create_time = 0;
			info->modified_time = info->create_time;
			info->file_size = uint32_t(data.size());
			info->owner_id = 0;
			info->priv = false;

			reply->add(info);
		}

		reply->send();
	}

	void bdStorage::get_publisher_file(service_server* server, byte_buffer* buffer)
	{
		std::string filename;
		buffer->read_string(&filename);

#ifdef DEBUG
		printf("[DW]: [bdStorage]: loading publisher file: %s\n", filename.data());
#endif

		std::string data;

		if (this->load_publisher_resource(filename, data))
		{
#ifdef DEBUG
			printf("[DW]: [bdStorage]: sending publisher file: %s, size: %lld\n", filename.data(), data.size());
#endif

			auto reply = server->create_reply(this->task_id());
			reply->add(new bdFileData(data));
			reply->send();
		}
		else
		{
			server->create_reply(this->task_id(), game::BD_NO_FILE)->send();
		}
	}

	std::string bdStorage::get_user_file_path(const std::string& name)
	{
		return "players2/user/" + name;
	}

	void bdStorage::set_user_file(service_server* server, byte_buffer* buffer) const
	{
		bool priv;
		uint64_t owner;
		std::string game, filename, data;

		buffer->read_string(&game);
		buffer->read_string(&filename);
		buffer->read_bool(&priv);
		buffer->read_blob(&data);
		buffer->read_uint64(&owner);

		const auto path = get_user_file_path(filename);
		utils::io::write_file(path, data);

		auto* info = new bdFileInfo;

		info->file_id = *reinterpret_cast<const uint64_t*>(utils::cryptography::sha1::compute(filename).data());
		info->filename = filename;
		info->create_time = uint32_t(time(nullptr));
		info->modified_time = info->create_time;
		info->file_size = uint32_t(data.size());
		info->owner_id = owner;
		info->priv = priv;

		auto reply = server->create_reply(this->task_id());
		reply->add(info);
		reply->send();
	}

	void bdStorage::get_user_file(service_server* server, byte_buffer* buffer) const
	{
		uint64_t owner{};
		std::string game, filename, platform, data;

		buffer->read_string(&game);
		buffer->read_string(&filename);
		buffer->read_uint64(&owner);
		buffer->read_string(&platform);

#ifdef DEBUG
		printf("[DW]: [bdStorage]: user file: %s, %s, %s\n", game.data(), filename.data(), platform.data());
#endif

		const auto path = get_user_file_path(filename);
		if (utils::io::read_file(path, &data))
		{
			auto reply = server->create_reply(this->task_id());
			reply->add(new bdFileData(data));
			reply->send();
		}
		else
		{
			server->create_reply(this->task_id(), game::BD_NO_FILE)->send();
		}
	}

	void bdStorage::unk13(service_server* server, byte_buffer* buffer) const
	{
		// TODO:
		auto reply = server->create_reply(this->task_id());
		reply->send();
	}
}
