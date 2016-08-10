#ifndef BOT_DATA__STRUCTURES_GUILDMEMBER
#define BOT_DATA__STRUCTURES_GUILDMEMBER

#include <string>
#include <vector>

#include "../json/json.hpp"

#include "User.hpp"
#include "Role.hpp"

namespace DiscordObjects {
	class GuildMember {
	public:
		GuildMember();
		GuildMember(json data, User *user);

		void load_from_json(json data);
		std::string to_debug_string();

		bool operator==(GuildMember rhs);

		User *user;
		std::string nick;
		std::vector<Role *> roles;
		std::string joined_at; // TODO: better type
		bool deaf;
		bool mute;
	};

	inline GuildMember::GuildMember() {
		user = nullptr;
		nick = joined_at = "null";
		deaf = false;
		mute = false;
	}

	inline GuildMember::GuildMember(json data, User *user) : GuildMember() {
		this->user = user;
		load_from_json(data);
	}

	inline void GuildMember::load_from_json(json data) {
		nick = data.value("nick", "null");
		joined_at = data.value("joined_at", "null");
		deaf = data.value("deaf", false);
		mute = data.value("mute", false);
	}

	inline std::string GuildMember::to_debug_string() {
		return "**__GuildMember " + user->id + "__**"
			+ "\n**mention:** <@" + user->id + "> / " + user->username + "#" + user->discriminator
			+ "\n**bot:** " + std::to_string(user->bot)
			+ "\n**mfa_enabled:** " + std::to_string(user->mfa_enabled)
			+ "\n**avatar:** " + user->avatar
			+ "\n**status:** " + user->status
			+ "\n**game name:** " + user->game
			+ "\n**nick:** " + nick
			+ "\n**joined_at:** " + joined_at
			+ "\n**deaf:** " + std::to_string(deaf)
			+ "\n**mute:** " + std::to_string(mute);
	}

	inline bool GuildMember::operator==(GuildMember rhs) {
		return user->id == rhs.user->id;
	}
}

#endif