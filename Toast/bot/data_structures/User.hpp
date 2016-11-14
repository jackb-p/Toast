#ifndef BOT_DATA__STRUCTURES_USER
#define BOT_DATA__STRUCTURES_USER

#include <string>
#include <vector>

#include "../json/json.hpp"

using json = nlohmann::json;

namespace DiscordObjects {
	/*
	==================================================== USER OBJECT =======================================================
	Field			Type			Description														Required OAuth2 Scope
	------------------------------------------------------------------------------------------------------------------------
	id				snowflake		the users id													identify
	username		string			the users username, not unique across the platform				identify
	discriminator	string			the users 4-digit discord-tag									identify
	avatar			string			the users avatar hash											identify
	bot				bool			whether the user belongs to a OAuth2 application				identify
	mfa_enabled		bool			whether the user has two factor enabled on their account		identify
	verified		bool			whether the email on this account has been verified				email
	email			string			the users email													email
	*/

	class User {
	public:
		User();
		User(json data);

		void load_from_json(json data);

		bool operator==(User rhs);

		std::string id;
		std::string username;
		std::string discriminator;
		std::string avatar;
		bool bot;
		bool mfa_enabled;

		// presence
		std::string game;
		std::string status;

		std::vector<std::string> guilds;
	};

	inline User::User() {
		id = username = discriminator = avatar = game = "null";
		status = "offline";
		bot = mfa_enabled = false;
	}

	inline User::User(json data) : User() {
		load_from_json(data);
	}

	inline void User::load_from_json(json data) {
		id = data.value("id", "null");
		username = data.value("username", "null");
		discriminator = data.value("discriminator", "null");
		avatar = data.value("avatar", "null");
		bot = data.value("bot", false);
		mfa_enabled = data.value("mfa_enabled", false);
	}

	inline bool User::operator==(User rhs) {
		return id == rhs.id && id != "null";
	}
}

#endif