#include "GatewayHandler.hpp"

#include <boost/algorithm/string.hpp>

#include "DiscordAPI.hpp"
#include "Logger.hpp"
#include "data_structures/GuildMember.hpp"

extern std::string bot_token;

GatewayHandler::GatewayHandler() {
	last_seq = 0;

	CommandHelper::init();
}

void GatewayHandler::handle_data(std::string data, client &c, websocketpp::connection_hdl &hdl) {
	json decoded = json::parse(data);

	int op = decoded["op"];

	switch (op) {
	case 0: // Event dispatch
		on_dispatch(decoded, c, hdl);
		break;
	case 10: // Hello
		on_hello(decoded, c, hdl);
		break;
	case 11:
		Logger::write("Heartbeat acknowledged", Logger::LogLevel::Debug);
		break;
	}
}

void GatewayHandler::send_heartbeat(client *c, websocketpp::connection_hdl hdl, int interval) {
	while (true) {
		boost::this_thread::sleep(boost::posix_time::milliseconds(interval));
		if (!c) {
			Logger::write("[send_heartbeat] Client pointer is null", Logger::LogLevel::Severe);
			break;
		}
		else if (c->stopped()) {
			break;
		}


		json heartbeat = {
			{ "op", 1 },
			{ "d", last_seq }
		};

		c->send(hdl, heartbeat.dump(), websocketpp::frame::opcode::text);

		Logger::write("Sent heartbeat (seq: " + std::to_string(last_seq) + ")", Logger::LogLevel::Debug);
	}
}

void GatewayHandler::send_identify(client &c, websocketpp::connection_hdl &hdl) {
	json identify = {
		{ "op", 2 },
		{ "d", {
			{ "token", bot_token },
			{ "properties",{
				{ "$browser", "Microsoft Windows 10" },
				{ "$device", "TriviaBot-0.0" },
				{ "$referrer", "" },
				{ "$referring_domain", "" }
			} },
			{ "compress", false },
			{ "large_threshold", 250 },
			{ "shard",{ 0, 1 } }
		} }
	};

	c.send(hdl, identify.dump(), websocketpp::frame::opcode::text);
	Logger::write("Sent identify payload", Logger::LogLevel::Debug);
}

void GatewayHandler::send_request_guild_members(client &c, websocketpp::connection_hdl &hdl, std::string guild_id) {
	json request_guild_members = {
		{ "op", 8 },
		{ "d", {
			{ "guild_id", guild_id },
			{ "query", "" },
			{ "limit", 0 }
		} }
	};

	c.send(hdl, request_guild_members.dump(), websocketpp::frame::opcode::text);
	Logger::write("Requested guild members for " + guild_id, Logger::LogLevel::Debug);
}

void GatewayHandler::on_hello(json decoded, client &c, websocketpp::connection_hdl &hdl) {
	heartbeat_interval = decoded["d"]["heartbeat_interval"];

	Logger::write("Heartbeat interval: " + std::to_string(heartbeat_interval / 1000.0f) + " seconds", Logger::LogLevel::Debug);

	heartbeat_thread = std::make_unique<boost::thread>(boost::bind(&GatewayHandler::send_heartbeat, this, &c, hdl, heartbeat_interval));

	send_identify(c, hdl);
}

void GatewayHandler::on_dispatch(json decoded, client &c, websocketpp::connection_hdl &hdl) {
	last_seq = decoded["s"];
	std::string event_name = decoded["t"];
	json data = decoded["d"];

	if (event_name == "READY") {
		on_event_ready(data);
	}
	else if (event_name == "GUILD_CREATE") {
		on_event_guild_create(data);
	}
	else if (event_name == "GUILD_UPDATE") {
		on_event_guild_update(data);
	}
	else if (event_name == "GUILD_DELETE") {
		on_event_guild_delete(data);
	}
	else if (event_name == "GUILD_MEMBER_ADD") {
		on_event_guild_member_add(data);
	}
	else if (event_name == "GUILD_MEMBER_UPDATE") {
		on_event_guild_member_update(data);
	}
	else if (event_name == "GUILD_MEMBER_REMOVE") {
		on_event_guild_member_remove(data);
	}
	else if (event_name == "GUILD_ROLE_CREATE") {
		on_event_guild_role_create(data);
	}
	else if (event_name == "GUILD_ROLE_UPDATE") {
		on_event_guild_role_update(data);
	}
	else if (event_name == "GUILD_ROLE_DELETE") {
		on_event_guild_role_delete(data);
	}
	else if (event_name == "CHANNEL_CREATE") {
		on_event_channel_create(data);
	}
	else if (event_name == "CHANNEL_UPDATE") {
		on_event_channel_update(data);
	}
	else if (event_name == "CHANNEL_DELETE") {
		on_event_channel_delete(data);
	}
	else if (event_name == "MESSAGE_CREATE") {
		on_event_message_create(data);
	}
	else if (event_name == "PRESENCE_UPDATE") {
		on_event_presence_update(data);
	}
}

void GatewayHandler::on_event_ready(json data) {
	user_object.load_from_json(data["user"]);

	Logger::write("Sign-on confirmed. (@" + user_object.username + "#" + user_object.discriminator + ")", Logger::LogLevel::Info);
}

void GatewayHandler::on_event_presence_update(json data) {
	std::string user_id = data["user"]["id"];

	auto it = users.find(user_id);
	if (it != users.end()) {
		it->second.status = data.value("status", "offline");
		if (data["game"] == nullptr) {
			it->second.game = "null";
		}
		else {
			it->second.game = data["game"].value("name", "null");
		}
	}
	else {
		Logger::write("Tried to add presence for user " + user_id + " who doesn't exist", Logger::LogLevel::Warning);
	}
}

void GatewayHandler::on_event_guild_create(json data) {
	guilds[data["id"]] = DiscordObjects::Guild(data);
	DiscordObjects::Guild &guild = guilds[data["id"]];

	Logger::write("Received info for guild " + guild.id + ", now in " + std::to_string(guilds.size()) + " guild(s)", Logger::LogLevel::Info);

	int channels_added = 0, roles_added = 0, members_added = 0, presences_added = 0;

	for (json channel : data["channels"]) {
		std::string channel_id = channel["id"];
		channel["guild_id"] = guild.id;

		channels[channel_id] = DiscordObjects::Channel(channel);
		guilds[guild.id].channels.push_back(&channels[channel_id]);

		channels_added++;
	}
	for (json role : data["roles"]) {
		std::string role_id = role["id"];

		roles[role_id] = DiscordObjects::Role(role);
		guilds[guild.id].roles.push_back(&roles[role_id]);

		roles_added++;
	}
	for (json member : data["members"]) {
		std::string user_id = member["user"]["id"];

		auto it = users.find(user_id);
		if (it == users.end()) { // new user
			users[user_id] = DiscordObjects::User(member["user"]);
		}
		users[user_id].guilds.push_back(guild.id);

		DiscordObjects::GuildMember *guild_member = new DiscordObjects::GuildMember(member, &users[user_id]);
		for (std::string role_id : member["roles"]) {
			guild_member->roles.push_back(&roles[role_id]);
		}

		guilds[guild.id].members.push_back(guild_member);

		members_added++;
	}
	for (json presence : data["presences"]) {
		std::string user_id = presence["user"]["id"];

		auto it = users.find(user_id);
		if (it != users.end()) {
			it->second.status = presence.value("status", "offline");
			if (presence["game"] == nullptr) {
				it->second.game = "null";
			} else {
				it->second.game = presence["game"].value("name", "null");
			}

			presences_added++;
		}
		else {
			Logger::write("Tried to add presence for user " + user_id + " who doesn't exist", Logger::LogLevel::Warning);
		}
	}

	if (v8_instances.count(guild.id) == 0) {
		v8_instances[guild.id] = std::make_unique<V8Instance>(guild.id, &guilds, &channels, &users, &roles);
		Logger::write("Created v8 instance for guild " + guild.id, Logger::LogLevel::Debug);
	}

	Logger::write("Loaded " + std::to_string(channels_added) + " channels, " + std::to_string(roles_added)  + " roles and " 
		+ std::to_string(members_added) + " members (with " + std::to_string(presences_added) + " presences) to guild " + guild.id, Logger::LogLevel::Debug);
}

void GatewayHandler::on_event_guild_update(json data) {
	std::string guild_id = data["id"];

	guilds[guild_id].load_from_json(data);
	Logger::write("Updated guild " + guild_id, Logger::LogLevel::Debug);
}

void GatewayHandler::on_event_guild_delete(json data) {
	std::string guild_id = data["id"];
	bool unavailable = data.value("unavailable", false);

	if (unavailable) {
		Logger::write("Guild " + guild_id + " has become unavailable", Logger::LogLevel::Info);
		guilds[guild_id].unavailable = true;
	} else {
		int channels_removed = 0;
		for (auto it = channels.cbegin(); it != channels.cend();) {
			if (it->second.guild_id == guild_id) {
				channels.erase(it++);
				channels_removed++;
			} else {
				++it;
			}
		}

		guilds.erase(guilds.find(guild_id));
		Logger::write("Guild " + guild_id + " and " + std::to_string(channels_removed) + " channels removed", Logger::LogLevel::Info);
	}
}

void GatewayHandler::on_event_guild_member_add(json data) {
	std::string guild_id = data["guild_id"];
	std::string user_id = data["user"]["id"];

	auto it = users.find(user_id);
	if (it == users.end()) { // new user
		users[user_id] = DiscordObjects::User(data["user"]);
	}
	users[user_id].guilds.push_back(guild_id);

	DiscordObjects::GuildMember *guild_member = new DiscordObjects::GuildMember(data, &users[user_id]);
	for (std::string role_id : data["roles"]) {
		guild_member->roles.push_back(&roles[role_id]);
	}

	guilds[guild_id].members.push_back(guild_member);

	Logger::write("Added new member " + guild_member->user->id + " to guild " + guild_id, Logger::LogLevel::Debug);
}

void GatewayHandler::on_event_guild_member_update(json data) {
	std::string user_id = data["user"]["id"];
	DiscordObjects::Guild &guild = guilds[data["guild_id"]];

	auto it = std::find_if(guild.members.begin(), guild.members.end(), [user_id](DiscordObjects::GuildMember *member) {
		return user_id == member->user->id;
	});
	if (it != guild.members.end()) {
		bool nick_changed = false;
		size_t roles_change = 0;

		DiscordObjects::GuildMember *member = (*it);

		std::string nick = data.value("nick", "null");
		if (member->nick != nick) {
			member->nick = nick;
			nick_changed = true;
		}

		roles_change = member->roles.size();
		member->roles.clear(); // reset and re-fill, changing the differences is probably more expensive anyway.
		for (std::string role_id : data["roles"]) {
			member->roles.push_back(&roles[role_id]);
		}
		roles_change = member->roles.size() - roles_change;
		
		std::string debug_string = "Updated member " + user_id + " of guild " + guild.id;
		if (nick_changed) debug_string += ". Nick changed to " + nick;
		if (roles_change != 0) debug_string += ". No. of roles changed by " + std::to_string(roles_change);
		debug_string += ".";

		Logger::write(debug_string, Logger::LogLevel::Debug);
	}
	else {
		Logger::write("Tried to update member " + user_id + " (of guild " + guild.id + ") who does not exist.", Logger::LogLevel::Warning);
	}
}

void GatewayHandler::on_event_guild_member_remove(json data) {
	DiscordObjects::Guild &guild = guilds[data["guild_id"]];
	std::string user_id = data["user"]["id"];

	auto it = std::find_if(guild.members.begin(), guild.members.end(), [user_id](DiscordObjects::GuildMember *member) {
		return user_id == member->user->id;
	});
	if (it != guild.members.end()) {
		delete (*it);
		guild.members.erase(it);
		
		users[user_id].guilds.erase(std::remove(users[user_id].guilds.begin(), users[user_id].guilds.end(), guild.id));

		if (users[user_id].guilds.size() == 0) {
			users.erase(users.find(user_id));
			Logger::write("User " + user_id + " removed from guild " + guild.id + " and no longer visible, deleted.", Logger::LogLevel::Debug);
		}
		else {
			Logger::write("User " + user_id + " removed from guild " + guild.id, Logger::LogLevel::Debug);
		}
	}
	else {
		Logger::write("Tried to remove guild member " + user_id + " who doesn't exist", Logger::LogLevel::Warning);
	}
}

void GatewayHandler::on_event_guild_role_create(json data) {
	std::string role_id = data["role"]["id"];
	std::string guild_id = data["guild_id"];
	roles[role_id] = DiscordObjects::Role(data["role"]);

	guilds[guild_id].roles.push_back(&roles[role_id]);

	Logger::write("Created role " + role_id + " on guild " + guild_id, Logger::LogLevel::Debug);
}

void GatewayHandler::on_event_guild_role_update(json data) {
	std::string role_id = data["role"]["id"];

	roles[role_id].load_from_json(data["role"]);
}

void GatewayHandler::on_event_guild_role_delete(json data) {
	std::string role_id = data["role_id"];
	auto it = roles.find(role_id);

	if (it != roles.end()) {
		DiscordObjects::Role &role = roles[role_id];
		DiscordObjects::Guild &guild = guilds[data["guild_id"]];

		auto check_lambda = [role_id](const DiscordObjects::Role *r) {
			return r->id == role_id;
		};

		auto it2 = std::find_if(guild.roles.begin(), guild.roles.end(), check_lambda);
		if (it2 != guild.roles.end()) {
			guild.roles.erase(it2);
		}
		else {
			Logger::write("Tried to delete role " + role_id + " from guild " + guild.id + " but it doesn't exist there", Logger::LogLevel::Warning);
		}

		roles.erase(it);
		Logger::write("Deleted role " + role_id + " (guild " + guild.id + ").", Logger::LogLevel::Debug);
	}
	else {
		Logger::write("Tried to delete role " + role_id + " but it doesn't exist.", Logger::LogLevel::Warning);
	}
}

void GatewayHandler::on_event_channel_create(json data) {
	std::string channel_id = data["id"];
	std::string guild_id = data["guild_id"];

	channels[channel_id] = DiscordObjects::Channel(data);
	Logger::write("Added channel " + channel_id + " to channel list. Now " + std::to_string(channels.size()) + " channels stored", Logger::LogLevel::Debug);
	guilds[guild_id].channels.push_back(&channels[channel_id]);
	Logger::write("Added channel " + channel_id + " to guild " + guild_id + "'s list. Now " + std::to_string(guilds[guild_id].channels.size()) + " channels stored", Logger::LogLevel::Debug);
}

void GatewayHandler::on_event_channel_update(json data) {
	std::string channel_id = data["id"];

	auto it = channels.find(channel_id);
	if (it == channels.end()) {
		Logger::write("Got channel update for channel " + channel_id + " that doesn't exist. Creating channel instead.", Logger::LogLevel::Warning);
		on_event_channel_create(data);
	} else {
		channels[channel_id].load_from_json(data);
		Logger::write("Updated channel " + channel_id, Logger::LogLevel::Debug);
	}
}

void GatewayHandler::on_event_channel_delete(json data) {
	std::string channel_id = data["id"];
	std::string guild_id = data["guild_id"];

	auto it = channels.find(channel_id);
	if (it == channels.end()) {
		Logger::write("Tried to delete channel " + channel_id + " which doesn't exist", Logger::LogLevel::Warning);
	}
	else {
		auto it2 = std::find_if(guilds[guild_id].channels.begin(), guilds[guild_id].channels.begin(), [channel_id](const DiscordObjects::Channel *c) {
			return c->id == channel_id;
		});
		guilds[guild_id].channels.erase(it2);
		Logger::write("Removed channel " + channel_id + " from guild " + guild_id + "'s list. Now " 
			+ std::to_string(guilds[guild_id].channels.size()) + " channels stored", Logger::LogLevel::Debug);

		channels.erase(it);
		Logger::write("Removed channel " + channel_id + " from channel list. Now " + std::to_string(channels.size()) + " channels stored.", Logger::LogLevel::Debug);
	}
}

void GatewayHandler::on_event_message_create(json data) {
	std::string message = data["content"];

	DiscordObjects::Channel &channel = channels[data["channel_id"]];
	DiscordObjects::Guild &guild = guilds[channel.guild_id];
	DiscordObjects::User &sender = users[data["author"]["id"]];

	if (sender.bot) return;

	std::vector<std::string> words;
	boost::split(words, message, boost::is_any_of(" "));
	CommandHelper::Command custom_command;
	if (words[0] == "`trivia" || words[0] == "`t") {
		int questions = 10;
		int delay = 8;

		if (words.size() > 3) {
			DiscordAPI::send_message(channel.id, ":exclamation: Invalid arguments!");
			return;
		}
		else  if (words.size() > 1) {
			if (words[1] == "help" || words[1] == "h") {
				std::string help = "**Base command \\`t[rivia]**. Arguments:\n";
				help += "\\`trivia **{x}** **{y}**: Makes the game last **x** number of questions, optionally sets the time interval between hints to **y** seconds\n";
				help += "\\`trivia **stop**: stops the ongoing game.\n";
				help += "\\`trivia **help**: prints this message\n";

				DiscordAPI::send_message(channel.id, help);
				return;
			}
			else if (words[1] == "stop" || words[1] == "s") {
				if (games.find(channel.id) != games.end()) {
					delete_game(channel.id);
					return;
				}
			}
			else {
				try {
					questions = std::stoi(words[1]);
					if (words.size() == 3) {
						delay = std::stoi(words[2]);
					}
				}
				catch (std::invalid_argument e) {
					DiscordAPI::send_message(channel.id, ":exclamation: Invalid arguments!");
					return;
				}
			}
		}

		games[channel.id] = std::make_unique<TriviaGame>(this, channel.id, questions, delay);
		games[channel.id]->start();
	}
	else if (words[0] == "`guilds") {
		std::string m = "Guild List:\n";
		for (auto &gu : guilds) {
			m += "> " + gu.second.name + " (" + gu.second.id + ") Channels: " + std::to_string(gu.second.channels.size()) + "\n";
		}
		DiscordAPI::send_message(channel.id, m);
	}
	else if (words[0] == "`info") {
		DiscordAPI::send_message(channel.id, ":information_source: trivia-bot by Jack. <http://github.com/jackb-p/TriviaDiscord>");
	}
	else if (words[0] == "~js" && words.size() > 1) {
		DiscordObjects::GuildMember *member = *std::find_if(guild.members.begin(), guild.members.end(), [sender](DiscordObjects::GuildMember *m) {
			return sender.id == m->user->id;
		});
		std::string js = message.substr(4);
		auto it = v8_instances.find(channel.guild_id);
		if (it != v8_instances.end() && js.length() > 0) {
			it->second->exec_js(js, &channel, member);
		}
	}
	else if (words[0] == "~createjs" && words.size() > 1) {
		std::string args = message.substr(10);
		size_t seperator_loc = args.find("|");
		if (seperator_loc != std::string::npos) {
			std::string command_name = args.substr(0, seperator_loc);
			std::string script = args.substr(seperator_loc + 1);
			int result = CommandHelper::insert_command(channel.guild_id, command_name, script);
			switch (result) {
			case 0:
				DiscordAPI::send_message(channel.id, ":warning: Error!"); break;
			case 1:
				DiscordAPI::send_message(channel.id, ":new: Command `" + command_name + "` successfully created."); break;
			case 2:
				DiscordAPI::send_message(channel.id, ":arrow_heading_up: Command `" + command_name + "` successfully updated."); break;
			}
		}
	}
	else if (words[0] == "`shutdown" && sender.id == "82232146579689472") { // it me
		DiscordAPI::send_message(channel.id, ":zzz: Goodbye!");
		// TODO: without needing c, hdl - c.close(hdl, websocketpp::close::status::going_away, "`shutdown command used.");
	}
	else if (words[0] == "`debug") {
		if (words[1] == "channel" && words.size() == 3) {
			auto it = channels.find(words[2]);
			if (it != channels.end()) {
				DiscordAPI::send_message(channel.id, it->second.to_debug_string());
			}
			else {
				DiscordAPI::send_message(channel.id, ":question: Unrecognised channel.");
			}
		}
		else if (words[1] == "guild" && words.size() == 3) {
			auto it = guilds.find(words[2]);
			if (it != guilds.end()) {
				DiscordAPI::send_message(channel.id, it->second.to_debug_string());
			}
			else {
				DiscordAPI::send_message(channel.id, ":question: Unrecognised guild.");
			}
		}
		else if (words[1] == "member" && words.size() == 4) {
			auto it = guilds.find(words[2]);
			if (it != guilds.end()) {
				std::string user_id = words[3];

				auto it2 = std::find_if(it->second.members.begin(), it->second.members.end(), [user_id](DiscordObjects::GuildMember *member) {
					return user_id == member->user->id;
				});
				if (it2 != it->second.members.end()) {
					DiscordAPI::send_message(channel.id, (*it2)->to_debug_string());
				}
				else {
					DiscordAPI::send_message(channel.id, ":question: Unrecognised user.");
				}
			}
			else {
				DiscordAPI::send_message(channel.id, ":question: Unrecognised guild.");
			}
		}
		else if (words[1] == "role" && words.size() == 3) {
			auto it = roles.find(words[2]);
			if (it != roles.end()) {
				DiscordAPI::send_message(channel.id, it->second.to_debug_string());
			}
			else {
				DiscordAPI::send_message(channel.id, ":question: Unrecognised role.");
			}
		}
		else if (words[1] == "role" && words.size() == 4) {
			std::string role_name = words[3];

			auto it = guilds.find(words[2]);
			if (it != guilds.end()) {
				auto check_lambda = [role_name](DiscordObjects::Role *r) {
					return role_name == r->name;
				};

				auto it2 = std::find_if(it->second.roles.begin(), it->second.roles.end(), check_lambda);
				if (it2 != it->second.roles.end()) {
					DiscordAPI::send_message(channel.id, (*it2)->to_debug_string());
				}
				else {
					DiscordAPI::send_message(channel.id, ":question: Unrecognised role.");
				}
			}
			else {
				DiscordAPI::send_message(channel.id, ":question: Unrecognised guild.");
			}
		}
		else {
			DiscordAPI::send_message(channel.id, ":question: Unknown parameters.");
		}
	}
	else if (CommandHelper::get_command(channel.guild_id, words[0], custom_command)) {
		std::string args = "";
		if (message.length() > (words[0].length() + 1)) {
			args = message.substr(words[0].length() + 1);
		}

		auto it = v8_instances.find(channel.guild_id);
		if (it != v8_instances.end() && custom_command.script.length() > 0) {
			DiscordObjects::GuildMember *member = *std::find_if(guild.members.begin(), guild.members.end(), [sender](DiscordObjects::GuildMember *m) {
				return sender.id == m->user->id;
			});
			it->second->exec_js(custom_command.script, &channel, member, args);
		}
	}
	else if (games.find(channel.id) != games.end()) { // message received in channel with ongoing game
		games[channel.id]->handle_answer(message, sender);
	}
}

void GatewayHandler::delete_game(std::string channel_id) {
	auto it = games.find(channel_id);

	if (it != games.end()) {
		it->second->interrupt();
		// remove from map
		games.erase(it);
	} else {
		Logger::write("Tried to delete a game that didn't exist (channel_id: " + channel_id + ")", Logger::LogLevel::Warning);
	}
}