#ifndef BOT_DATA__STRUCTURES_CHANNEL
#define BOT_DATA__STRUCTURES_CHANNEL

#include <string>

#include "../json/json.hpp"

using json = nlohmann::json;

namespace DiscordObjects {
	/*
	=============================================================== CHANNEL OBJECT ==============================================================
	|Field					|Type			|Description																			|Present	|
	|-----------------------|---------------|---------------------------------------------------------------------------------------|-----------|
	|id						|snowflake		|the id of this channel (will be equal to the guild if it's the "general" channel)		|Always		|
	|guild_id				|snowflake		|the id of the guild																	|Always		|
	|name					|string			|the name of the channel (2-100 characters)												|Always		|
	|type					|string			|"text" or "voice"																		|Always		|
	|position				|integer		|the ordering position of the channel													|Always		|
	|is_private				|bool			|should always be false for guild channels												|Always		|
	|permission_overwrites	|array			|an array of overwrite objects															|Always		|
	|topic					|string			|the channel topic (0-1024 characters)													|Text only	|
	|last_message_id		|snowflake		|the id of the last message sent in this channel										|Text only	|
	|bitrate				|integer		|the bitrate (in bits) of the voice channel												|Voice only	|
	|user_limit				|integer		|the user limit of the voice channel													|Voice only	|
	---------------------------------------------------------------------------------------------------------------------------------------------
	*/

	class Channel {
	public:
		Channel();
		Channel(json data);

		void load_from_json(json data);

		bool operator==(Channel rhs);

		std::string id;
		std::string guild_id;
		std::string name;
		std::string type;
		int position;
		bool is_private;
		// TODO: Implement permission overwrites
		// std::vector<Permission_Overwrite> permission_overwrites;
		std::string topic;
		std::string last_message_id;
		int bitrate;
		int user_limit;
	};

	inline Channel::Channel() {
		id = guild_id = name = topic = last_message_id = "null";
		position = bitrate = user_limit = -1;
		is_private = false;
		type = "text";
	}

	inline Channel::Channel(json data) {
		load_from_json(data);
	}

	inline void Channel::load_from_json(json data) {
		id = data.value("id", "null");
		guild_id = data.value("guild_id", "null");
		name = data.value("name", "null");
		type = data.value("type", "text");
		position = data.value("position", -1);
		is_private = data.value("is_private", false);
		topic = data.value("topic", "null");
		last_message_id = data.value("last_message_id", "null");
		bitrate = data.value("bitrate", -1);
		user_limit = data.value("user_limit", -1);
	}

	inline bool Channel::operator==(Channel rhs) {
		return id == rhs.id && id != "null";
	}
}

#endif