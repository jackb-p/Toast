#ifndef BOT_DATA__STRUCTURES_ROLE
#define BOT_DATA__STRUCTURES_ROLE

#include <string>
#include <sstream>
#include <iomanip>

#include "../json/json.hpp"

using json = nlohmann::json;

namespace DiscordObjects {

	class Role {
	public:
		Role();
		Role(json data);

		void load_from_json(json data);
		std::string to_debug_string();

		bool operator==(Role rhs);

		std::string id;
		std::string name;
		int colour;
		bool hoist;
		int position;
		int permissions;
		bool managed;
		bool mentionable;
	};

	inline Role::Role() {
		id = "null";
		name = "null";
		colour = -1;
		hoist = false;
		position = -1;
		permissions = 0;
		managed = false;
		mentionable = false;
	}

	inline Role::Role(json data) {
		load_from_json(data);
	}

	inline void Role::load_from_json(json data) {
		id = data.value("id", "null");
		name = data.value("name", "null");
		colour = data.value("color", -1);
		hoist = data.value("hoist", false);
		position = data.value("position", -1);
		permissions = data.value("permissions", 0);
		managed = data.value("managed", false);
		mentionable = data.value("mentionable", false);
	}

	inline std::string Role::to_debug_string() {
		return "**__Role " + id + "__**"
			+ "\n**name:** " + name
			+ "\n**colour:** " + std::to_string(colour)
			+ "\n**hoist:** " + std::to_string(hoist)
			+ "\n**position:** " + std::to_string(position)
			+ "\n**permissions:** " + std::to_string(permissions)
			+ "\n**managed:** " + std::to_string(managed)
			+ "\n**mentionable:** " + std::to_string(mentionable);
	}

	inline bool Role::operator==(Role rhs) {
		return id == rhs.id;
	}


	/* permission values */
	enum class Permission {
		CreateInstantInvite = 0x00000001, // Allows creation of instant invites
		KickMembers = 0x00000002, // Allows kicking members
		BanMembers = 0x00000004, // Allows banning members
		Administrator = 0x00000008, // Allows all permissions and bypasses channel permission overwrites
		ManageChannels = 0x00000010, // Allows management and editing of channels
		ManageGuild = 0x00000020, // Allows management and editing of the guild
		ReadMessages = 0x00000400, // Allows reading messages in a channel.The channel will not appear for users without this permission
		SendMessages = 0x00000800, // Allows for sending messages in a channel.
		SendTTSMessages = 0x00001000, // Allows for sending of / tts messages
		ManageMessages = 0x00002000, // Allows for deletion of other users messages
		EmbedLinks = 0x00004000, // Links sent by this user will be auto - embedded
		AttachFiles = 0x00008000, // Allows for uploading images and files
		ReadMessageHistory = 0x00010000, // Allows for reading of message history
		MentionEveryone = 0x00020000, // Allows for using the @everyone tag to notify all users in a channel, and the @here tag to notify all online users in a channel
		Connect = 0x00100000, // Allows for joining of a voice channel
		Speak = 0x00200000, // Allows for speaking in a voice channel
		MuteMembers = 0x00400000, // Allows for muting members in a voice channel
		DeafenMembers = 0x00800000, // Allows for deafening of members in a voice channel
		MoveMembers = 0x01000000, // Allows for moving of members between voice channels
		UseVAD = 0x02000000, // Allows for using voice - activity - detection in a voice channel
		ChangeNickname = 0x04000000, // Allows for modification of own nickname
		ManageNicknames = 0x08000000, // Allows for modification of other users nicknames
		ManageRoles = 0x10000000 // Allows management and editing of roles
	};

	/* implement bitwise operators */
	inline Permission operator|(Permission lhs, Permission rhs) {
		return static_cast<Permission>(static_cast<int>(lhs) | static_cast<int>(rhs));
	}

	inline Permission operator|=(Permission &lhs, Permission rhs) {
		lhs = static_cast<Permission>(static_cast<int>(lhs) | static_cast<int>(rhs));
		return lhs;
	}
}

#endif