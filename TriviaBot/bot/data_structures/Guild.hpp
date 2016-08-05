#ifndef BOT_DATA__STRUCTURES_GUILD
#define BOT_DATA__STRUCTURES_GUILD

#include <string>
#include <vector>

#include "../json/json.hpp"

#include "Channel.hpp"
#include "User.hpp"
#include "Role.hpp"
#include "GuildMember.hpp"

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
		std::string to_debug_string();

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
		// TODO: Implement all guild fields
		// std::vector<?> voice_states
		// std::vector<?> emojis
		// std::vector<?> features
		bool unavailable;

		std::vector<Channel *> channels;
		std::vector<GuildMember> members;
		std::vector<Role *> roles;
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

		id = data.value("id", "null");
		name = data.value("name", "null");
		icon = data.value("icon", "null");
		splash = data.value("spash", "null");
		owner_id = data.value("owner_id", "null");
		region = data.value("region", "null");
		afk_channel_id = data.value("afk_channel_id", "null");
		afk_timeout = data.value("afk_timeout", -1);
		verification_level = data.value("verification_level", -1);
		unavailable = data.value("unavailable", false);
	}

	inline std::string Guild::to_debug_string() {
		return "**__Guild " + id + "__**"
			+ "\n**name:** " + name
			+ "\n**icon:** " + icon
			+ "\n**splash:** " + splash
			+ "\n**owner_id:** " + owner_id
			+ "\n**region:** " + region
			+ "\n**afk_channel_id:** " + afk_channel_id
			+ "\n**afk_timeout:** " + std::to_string(afk_timeout)
			+ "\n**verification_level:** " + std::to_string(verification_level)
			+ "\n**unavailable:** " + std::to_string(unavailable)
			+ "\n**channels:** " + std::to_string(channels.size())
			+ "\n**roles:** " + std::to_string(roles.size())
			+ "\n**members:** " + std::to_string(members.size());
	}

	inline bool Guild::operator==(Guild rhs) {
		return id == rhs.id && id != "null";
	}
}

#endif