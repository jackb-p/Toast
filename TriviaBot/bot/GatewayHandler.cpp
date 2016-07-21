#include "GatewayHandler.hpp"

#include <boost/algorithm/string.hpp>

#include "APIHelper.hpp"
#include "data_structures/User.hpp"

extern std::string bot_token;

GatewayHandler::GatewayHandler() {
	last_seq = 0;

	ah = new APIHelper();
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
		c.get_alog().write(websocketpp::log::alevel::app, "Heartbeat acknowledged.");
		break;
	}
}

void GatewayHandler::heartbeat(client *c, websocketpp::connection_hdl hdl, int interval) {
	while (true) {
		boost::this_thread::sleep(boost::posix_time::milliseconds(interval));

		json heartbeat = {
			{ "op", 1 },
			{ "d", last_seq }
		};

		c->send(hdl, heartbeat.dump(), websocketpp::frame::opcode::text);

		c->get_alog().write(websocketpp::log::alevel::app, "Sent heartbeat. (seq: " + std::to_string(last_seq) + ")");
	}
}

void GatewayHandler::on_hello(json decoded, client &c, websocketpp::connection_hdl &hdl) {
	heartbeat_interval = decoded["d"]["heartbeat_interval"];

	c.get_alog().write(websocketpp::log::alevel::app, "Heartbeat interval: " + std::to_string(heartbeat_interval / 1000.0f) + " seconds");

	heartbeat_thread = std::make_unique<boost::thread>(boost::bind(&GatewayHandler::heartbeat, this, &c, hdl, heartbeat_interval));

	identify(c, hdl);
}

void GatewayHandler::on_dispatch(json decoded, client &c, websocketpp::connection_hdl &hdl) {
	last_seq = decoded["s"];
	std::string event_name = decoded["t"];
	json data = decoded["d"];

	if (event_name == "READY") {
		user_object.load_from_json(data["user"]);

		c.get_alog().write(websocketpp::log::alevel::app, "Sign-on confirmed. (@" + user_object.username + "#" + user_object.discriminator + ")");
	}
	else if (event_name == "GUILD_CREATE") {
		std::string guild_id = data["id"];
		try {
			guilds[guild_id] = std::make_unique<DiscordObjects::Guild>(data);
		}
		catch (std::domain_error err) {
			// this doesn't even work
			c.get_alog().write(websocketpp::log::elevel::rerror, "Domain error");
		}
		

		c.get_alog().write(websocketpp::log::alevel::app, "Loaded guild: " + guilds[guild_id]->name);
		
		for (json channel : data["channels"]) {
			std::string channel_id = channel["id"];
			channel["guild_id"] = guild_id;
			// create channel obj, add to overall channel list
			channels[channel_id] = std::make_shared<DiscordObjects::Channel>(channel);
			// add ptr to said channel list to guild's channel list
			guilds[guild_id]->channels.push_back(std::shared_ptr<DiscordObjects::Channel>(channels[channel_id]));
		}
	}
	else if (event_name == "TYPING_START") {}
	else if (event_name == "MESSAGE_CREATE") {
		std::string message = data["content"];
		auto channel = channels[data["channel_id"]];

		DiscordObjects::User sender(data["author"]);

		std::vector<std::string> words;
		boost::split(words, message, boost::is_any_of(" "));
		if (words[0] == "`trivia" || words[0] == "`t") {
			int questions = 10;
			int delay = 8;

			if (words.size() > 3) {
				ah->send_message(channel->id, ":exclamation: Invalid arguments!");
				return;
			}
			else  if(words.size() > 1) {
				if (words[1] == "help" || words[1] == "h") {
					std::string help = "**Base command \\`t[rivia]**. Arguments:\n";
					help += "\\`trivia **{x}** **{y}**: Makes the game last **x** number of questions, optionally sets the time interval between hints to **y** seconds\n";
					help += "\\`trivia **stop**: stops the ongoing game.\n";
					help += "\\`trivia **help**: prints this message\n";

					ah->send_message(channel->id, help);
					return;
				}
				else if (words[1] == "stop" || words[1] == "s") {
					if (games.find(channel->id) != games.end()) {
						delete_game(channel->id);
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
						ah->send_message(channel->id, ":exclamation: Invalid arguments!");
						return;
					}
				}
			}

			games[channel->id] = std::make_unique<TriviaGame>(this, ah, channel->id, questions, delay);
			games[channel->id]->start();
		} 
		else if (words[0] == "`channels") {
			std::string m = "Channel List:\n";
			for (auto ch : channels) {
				m += "> " + ch.second->name + " (" + ch.second->id + ") [" + ch.second->type + "] Guild: " 
					+ guilds[ch.second->guild_id]->name + " (" + ch.second->guild_id + ")\n";
			}
			ah->send_message(channel->id, m);
		}
		else if (words[0] == "`guilds") {
			std::string m = "Guild List:\n";
			for (auto &gu : guilds) {
				m += "> " + gu.second->name + " (" + gu.second->id + ") Channels: " + std::to_string(gu.second->channels.size()) + "\n";
			}
			ah->send_message(channel->id, m);
		}
		else if (games.find(channel->id) != games.end()) { // message received in channel with ongoing game
			games[channel->id]->handle_answer(message, sender);
		}
	}
}

void GatewayHandler::identify(client &c, websocketpp::connection_hdl &hdl) {
	json identify = {
		{ "op", 2 },
		{ "d", {
			{ "token", bot_token },
			{ "properties", {
				{ "$browser", "Microsoft Windows 10" },
				{ "$device", "TriviaBot-0.0" },
				{ "$referrer", "" },
				{ "$referring_domain", "" }
			} },
			{ "compress", false },
			{ "large_threshold", 250 },
			{ "shard", { 0, 1 } }
		} }
	};

	c.send(hdl, identify.dump(), websocketpp::frame::opcode::text);
	c.get_alog().write(websocketpp::log::alevel::app, "Sent identify payload.");
}

void GatewayHandler::delete_game(std::string channel_id) {
	auto it = games.find(channel_id);

	if (it != games.end()) {
		it->second->interrupt();
		// remove from map
		games.erase(it);
	} else {
		std::cerr << "Tried to delete a game that didn't exist.";
	}
}