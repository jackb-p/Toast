#include "GatewayHandler.hpp"

#include <boost/algorithm/string.hpp>

#include "APIHelper.hpp"
#include "data_structures/User.hpp"

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
	default:
		std::cout << data << std::endl;
	}
}

void GatewayHandler::heartbeat(websocketpp::lib::error_code const & ec, client *c, websocketpp::connection_hdl *hdl) {
	json heartbeat = {
		{ "op", 1 },
		{ "d", last_seq }
	};

	c->send(*hdl, heartbeat.dump(), websocketpp::frame::opcode::text);

	c->set_timer(heartbeat_interval, websocketpp::lib::bind(
		&GatewayHandler::heartbeat,
		this,
		websocketpp::lib::placeholders::_1,
		c,
		hdl
	));

	c->get_alog().write(websocketpp::log::alevel::app, "Sent heartbeat. (seq: " + std::to_string(last_seq) + ")");
}

void GatewayHandler::on_hello(json decoded, client &c, websocketpp::connection_hdl &hdl) {
	heartbeat_interval = decoded["d"]["heartbeat_interval"];

	c.get_alog().write(websocketpp::log::alevel::app, "Heartbeat interval: " + std::to_string((float)heartbeat_interval / 1000) + " seconds");

	c.set_timer(heartbeat_interval, websocketpp::lib::bind(
		&GatewayHandler::heartbeat,
		this,
		websocketpp::lib::placeholders::_1,
		&c,
		&hdl
	));

	identify(c, hdl);
}

void GatewayHandler::on_dispatch(json decoded, client &c, websocketpp::connection_hdl &hdl) {
	last_seq = decoded["s"];
	std::string event_name = decoded["t"];
	json data = decoded["d"];

	c.get_alog().write(websocketpp::log::alevel::app, "Received event: " + event_name + " (new seq value: " + std::to_string(last_seq) + ")");

	if (event_name == "READY") {
		user_object.load_from_json(data["user"]);

		c.get_alog().write(websocketpp::log::alevel::app, "Sign-on confirmed. (@" + user_object.username + "#" + user_object.discriminator + ")");

		c.get_alog().write(websocketpp::log::alevel::app, data.dump(4));
	}
	else if (event_name == "GUILD_CREATE") {
		std::string guild_id = data["id"];
		guilds[guild_id] = std::make_unique<DiscordObjects::Guild>(data);
		
		for (json channel : data["channels"]) {
			std::string channel_id = channel["id"];
			channel["guild_id"] = guild_id;
			// create channel obj, add to overall channel list
			channels[channel_id] = std::make_unique<DiscordObjects::Channel>(channel);
			// add ptr to said channel list to guild's channel list
			guilds[guild_id]->channels.push_back(std::shared_ptr<DiscordObjects::Channel>(channels[channel_id]));
		}

		c.get_alog().write(websocketpp::log::alevel::app, data.dump(4));
	}
	else if (event_name == "TYPING_START") {}
	else if (event_name == "MESSAGE_CREATE") {
		std::string message = data["content"];
		auto channel = channels[data["channel_id"]];

		DiscordObjects::User sender(data["author"]);

		c.get_alog().write(websocketpp::log::alevel::app, "Message received: " + message + " $" + channel->name + " ^" + channel->id);

		std::vector<std::string> words;
		boost::split(words, message, boost::is_any_of(" "));
		if (games.find(channel->id) != games.end()) { // message received in channel with ongoing game
			games[channel->id]->handle_answer(message, sender);
		} else if (words[0] == "`trivia" || words[0] == "`t") {
			int questions = 10;
			if (words.size() == 2) {
				try {
					questions = std::stoi(words[1]);
				} catch (std::invalid_argument e) {
					ah->send_message(channel->id, ":exclamation: Invalid arguments!");
				}
			} else if (words.size() > 2) {
				ah->send_message(channel->id, ":exclamation: Invalid arguments!");
			}

			games[channel->id] = std::make_unique<TriviaGame>(this, ah, channel->id, questions);
			games[channel->id]->start();
		}
		else if (words[0] == "`channels") {
			std::string m = "Channel List:\n";
			for (auto ch : channels) {
				m += "> " + ch.second->name + " (" + ch.second->id + ") [" + ch.second->type + "] Guild: " 
					+ guilds[ch.second->guild_id]->name + " (" + ch.second->guild_id + ")\n";
			}
			ah->send_message(channel->id, m);
		} else if (words[0] == "`guilds") {
			std::string m = "Guild List:\n";
			for (auto &gu : guilds) {
				m += "> " + gu.second->name + " (" + gu.second->id + ") Channels: " + std::to_string(gu.second->channels.size()) + "\n";
			}
			ah->send_message(channel->id, m);
		}
		c.get_alog().write(websocketpp::log::alevel::app, data.dump(2));
	}
	//c.get_alog().write(websocketpp::log::alevel::app, decoded.dump(2));
}

void GatewayHandler::identify(client &c, websocketpp::connection_hdl &hdl) {
	json identify = {
		{ "op", 2 },
		{ "d",{
			{ "token", TOKEN },
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
	c.get_alog().write(websocketpp::log::alevel::app, "Sent identify payload.");
}

void GatewayHandler::delete_game(std::string channel_id) {
	auto it = games.find(channel_id);

	if (it != games.end()) {
		// remove from map
		games.erase(it);
	} else {
		std::cerr << "Tried to delete a game that didn't exist.";
	}
}