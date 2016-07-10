#ifndef BOT_DATA__STRUCTURES_Guild
#define BOT_DATA__STRUCTURES_Guild

#include <string>
#include <memory>

#include "../json/json.hpp"

#include "Channel.hpp"
#include "User.hpp"

using json = nlohmann::json;

namespace DiscordObjects {
	/*
	=================================== GUILD OBJECT ====================================
	|Field				|Type			|Description									|
	|-------------------|---------------|-----------------------------------------------|
	|id					|snowflake		|guild id										|
	|name				|string			|guild name (2-100 characters)					|
	|icon				|string			|icon hash										|
	|splash				|string			|splash hash									|
	|owner_id			|snowflake		|id of owner									|
	|region				|string			|{voice_region.id}								|
	|afk_channel_id		|snowflake		|id of afk channel								|
	|afk_timeout		|integer		|afk timeout in seconds							|
	|embed_enabled		|bool			|is this guild embeddable (e.g. widget)			|
	|embed_channel_id	|snowflake		|id of embedded channel							|
	|verification_level	|integer		|level of verification							|
	|voice_states		|array			|array of voice state objects (w/o guild_id)	|
	|roles				|array			|array of role objects							|
	|emojis				|array			|array of emoji objects							|
	|features			|array			|array of guild features						|
	-------------------------------------------------------------------------------------

	Custom fields:
	------------------------------------------------------------------------------------
	|Field				|Type			|Description									|
	|-------------------|---------------|-----------------------------------------------|
	|channels			|array			|array of channel object ptrs                   |
	|users				|array			|array of user objects ptrs                     |
	-------------------------------------------------------------------------------------
	*/

	class Guild {
	public:
		Guild();
		Guild(json data);

		void load_from_json(json data);

		bool operator==(Guild rhs);

		std::string id;
		std::string	name;
		std::string	icon;
		std::string	splash;
		std::string	owner_id;
		std::string	region;
		std::string	afk_channel_id;
		int	        afk_timeout;
		// bool        embed_enabled;
		// std::string	embed_channel_id;
		int         verification_level;
		// TODO: Implement all guil fields
		// std::vector<?> voice_states
		// std::vector<?> roles
		// std::vector<?> emojis
		// std::vector<?> features

		std::vector<std::shared_ptr<Channel>> channels;
		//std::vector<std::unique_ptr<DiscordObjects::User>>    users;
	};

	inline Guild::Guild() {
		id = name = icon = splash = owner_id = region = afk_channel_id = "null";
		afk_timeout = verification_level = -1;
	}

	inline Guild::Guild(json data) {
		load_from_json(data);
	}

	inline void Guild::load_from_json(json data) {
		Guild();
		std::cout << data.dump(4) << std::endl;


		id = data.value("id", "null");
		name = data.value("name", "null");
		icon = data.value("icon", "null");
		splash = data.value("spash", "null");
		owner_id = data.value("owner_id", "null");
		region = data.value("region", "null");
		afk_channel_id = data.value("afk_channel_id", "null");
		afk_timeout = data.value("afk_timeout", -1);
		verification_level = data.value("verification_level", -1);
	}

	inline bool Guild::operator==(Guild rhs) {
		return id == rhs.id && id != "null";
	}
}

#endif