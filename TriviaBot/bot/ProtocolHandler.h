#include "json/json.hpp"

#include <websocketpp/client.hpp>
#include <iostream>

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
using json = nlohmann::json;

/************    Opcodes   **************************************************************************************************
* Code	|	Name					|	Description																			*
* --------------------------------------------------------------------------------------------------------------------------*
* 0		|	Dispatch				|	dispatches an event																	*
* 1		|	Heartbeat				|	used for ping checking																*
* 2		|	Identify				|	used for client handshake															*
* 3		|	Status Update			|	used to update the client status													*
* 4		|	Voice State Update		|	used to join/move/leave voice channels												*
* 5		|	Voice Server Ping		|	used for voice ping checking														*
* 6		|	Resume					|	used to resume a closed connection													*
* 7		|	Reconnect				|	used to tell clients to reconnect to the gateway									*
* 8		|	Request Guild Members	|	used to request guild members														*
* 9		|	Invalid Session			|	used to notify client they have an invalid session id								*
* 10	|	Hello					|	sent immediately after connecting, contains heartbeat and server debug information	*
* 11	|	Heartback ACK			|	sent immediately following a client heartbeat that was received						*
*****************************************************************************************************************************/

class ProtocolHandler {
public:
	ProtocolHandler() {
		last_seq = 0;
	}

	void handle_data(std::string data, client &c, websocketpp::connection_hdl &hdl) {
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

	void heartbeat(websocketpp::lib::error_code const & ec, client *c, websocketpp::connection_hdl *hdl) {
		json heartbeat = {
			{ "op", 1 },
			{ "d", last_seq }
		};

		c->send(*hdl, heartbeat.dump(), websocketpp::frame::opcode::text);

		c->set_timer(heartbeat_interval, websocketpp::lib::bind(
			&ProtocolHandler::heartbeat,
			this,
			websocketpp::lib::placeholders::_1,
			c,
			hdl
		));

		c->get_alog().write(websocketpp::log::alevel::app, "Sent heartbeat. (seq: " + std::to_string(last_seq) + ")");
	}

	void on_hello(json decoded, client &c, websocketpp::connection_hdl &hdl) {
		heartbeat_interval = decoded["d"]["heartbeat_interval"];

		c.get_alog().write(websocketpp::log::alevel::app, "Heartbeat interval: " + std::to_string((float)heartbeat_interval / 1000) + " seconds");

		c.set_timer(heartbeat_interval, websocketpp::lib::bind(
			&ProtocolHandler::heartbeat,
			this,
			websocketpp::lib::placeholders::_1,
			&c,
			&hdl
		));

		identify(c, hdl);
	}

	void on_dispatch(json decoded, client &c, websocketpp::connection_hdl &hdl) {
		last_seq = decoded["s"];
		std::string event_name = decoded["t"];
		json data = decoded["d"];

		c.get_alog().write(websocketpp::log::alevel::app, "Received event: " + event_name + " (new seq value: " + std::to_string(last_seq) + ")");

		if (event_name == "READY") {
			user_object = data["user"];

			std::string username = user_object["username"];
			std::string discriminator = user_object["discriminator"];
			c.get_alog().write(websocketpp::log::alevel::app, "Sign-on confirmed. (@" + username + "#" + discriminator + ")");

			c.get_alog().write(websocketpp::log::alevel::app, data.dump(4));
		}
		else if (event_name == "GUILD_CREATE") {
			std::string guild_name = data["name"];
			std::string guild_id = data["id"];

			guilds[guild_id] = data;

			for (json &element : data["channels"]) {
				if (element["type"] == "text"){
					channels[element["id"]] = element;
				}
			}

			c.get_alog().write(websocketpp::log::alevel::app, "Guild joined: " + guild_name);
		}
		else if (event_name == "TYPING_START") {}
		else if (event_name == "MESSAGE_CREATE") {
			std::string message = data["content"];
			json channel = channels[data["channel_id"]];
			std::string channel_name = channel["name"];
			std::string channel_id = data["channel_id"];

			c.get_alog().write(websocketpp::log::alevel::app, "Message received: " + message + " $" + channel_name + " ^" + channel_id);

			if (message == "`trivia" || message == "`t") {

			}
		}
		//c.get_alog().write(websocketpp::log::alevel::app, decoded.dump(2));
	}

	json create_message(json user, std::string channel_id) {

	}

	void identify(client &c, websocketpp::connection_hdl &hdl) {
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

private:
	int last_seq;
	int heartbeat_interval;

	const int protocol_version = 5;
	const std::string bot_token = "MTk5NjU3MDk1MjU4MTc3NTM5.ClyBNQ.15qTa-XBKRtGNMMYeXCrU50GhWE";

	std::map<std::string, json> guilds;
	std::map<std::string, json> channels;
	json user_object;
};