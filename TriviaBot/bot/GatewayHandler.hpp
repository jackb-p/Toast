#ifndef BOT_GATEWAYHANDLER
#define BOT_GATEWAYHANDLER

#include <map>
#include <string>

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include "json/json.hpp"

#include "TriviaGame.hpp"
#include "data_structures/User.hpp"
#include "data_structures/Guild.hpp"
#include "data_structures/Channel.hpp"

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

class TriviaGame;
class APIHelper;

class GatewayHandler {
public:
	GatewayHandler();

	void handle_data(std::string data, client &c, websocketpp::connection_hdl &hdl);

	void heartbeat(websocketpp::lib::error_code const & ec, client *c, websocketpp::connection_hdl *hdl);

	void on_hello(json decoded, client &c, websocketpp::connection_hdl &hdl);

	void on_dispatch(json decoded, client &c, websocketpp::connection_hdl &hdl);

	void identify(client &c, websocketpp::connection_hdl &hdl);

	void delete_game(std::string channel_id);

private:
	int last_seq;
	int heartbeat_interval;

	const int protocol_version = 5;
	const std::string TOKEN = "MTk5NjU3MDk1MjU4MTc3NTM5.ClyBNQ.15qTa-XBKRtGNMMYeXCrU50GhWE";

	// bot's user obj
	DiscordObjects::User user_object;

	// <id, ptr to data>
	std::map<std::string, std::unique_ptr<DiscordObjects::Guild>> guilds;
	// channels pointers are shared pointers, held here but also in guild objects
	std::map<std::string, std::shared_ptr<DiscordObjects::Channel>> channels;

	// <channel_id, game obj>
	std::map<std::string, std::unique_ptr<TriviaGame>> games;

	APIHelper *ah;
};

#endif