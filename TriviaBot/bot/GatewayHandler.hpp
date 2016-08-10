#ifndef BOT_GATEWAYHANDLER
#define BOT_GATEWAYHANDLER

#include <map>
#include <string>

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include "json/json.hpp"

#include "TriviaGame.hpp"
#include "js/CommandHelper.hpp"
#include "js/V8Instance.hpp"
#include "data_structures/User.hpp"
#include "data_structures/Guild.hpp"
#include "data_structures/Channel.hpp"
#include "data_structures/Role.hpp"

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

	void delete_game(std::string channel_id);

private:
	int last_seq;
	int heartbeat_interval;

	/* payload dispatchers */
	void send_heartbeat(client *c, websocketpp::connection_hdl hdl, int interval);
	void send_identify(client &c, websocketpp::connection_hdl &hdl);
	void send_request_guild_members(client &c, websocketpp::connection_hdl &hdl, std::string guild_id); // not sure if required atm

	/* payload handlers */
	void on_hello(json decoded, client &c, websocketpp::connection_hdl &hdl);
	void on_dispatch(json decoded, client &c, websocketpp::connection_hdl &hdl);

	/* misc events */
	void on_event_ready(json data); // https://discordapp.com/developers/docs/topics/gateway#ready
	void on_event_presence_update(json data); // https://discordapp.com/developers/docs/topics/gateway#presence-update

	/* guild events */
	void on_event_guild_create(json data); // https://discordapp.com/developers/docs/topics/gateway#guild-create
	void on_event_guild_update(json data); // https://discordapp.com/developers/docs/topics/gateway#guild-update
	void on_event_guild_delete(json data); // https://discordapp.com/developers/docs/topics/gateway#guild-delete
	void on_event_guild_member_add(json data); // https://discordapp.com/developers/docs/topics/gateway#guild-member-add
	void on_event_guild_member_update(json data); // https://discordapp.com/developers/docs/topics/gateway#guild-member-update
	void on_event_guild_member_remove(json data); // https://discordapp.com/developers/docs/topics/gateway#guild-member-remove
	void on_event_guild_role_create(json data); // https://discordapp.com/developers/docs/topics/gateway#guild-role-create
	void on_event_guild_role_update(json data); // https://discordapp.com/developers/docs/topics/gateway#guild-role-update
	void on_event_guild_role_delete(json data); // https://discordapp.com/developers/docs/topics/gateway#guild-role-delete

	/* channel events */
	void on_event_channel_create(json data); // https://discordapp.com/developers/docs/topics/gateway#channel-create
	void on_event_channel_update(json data); // https://discordapp.com/developers/docs/topics/gateway#channel-update
	void on_event_channel_delete(json data); // https://discordapp.com/developers/docs/topics/gateway#channel-delete

	/* message events */
	void on_event_message_create(json data); // https://discordapp.com/developers/docs/topics/gateway#message-create

	const int protocol_version = 5;

	// bot's user obj
	DiscordObjects::User user_object;

	/* <id, obj> */
	std::map<std::string, DiscordObjects::Guild> guilds;
	std::map<std::string, DiscordObjects::Channel> channels;
	std::map<std::string, DiscordObjects::User> users;
	std::map<std::string, DiscordObjects::Role> roles;

	// <channel_id, game obj>
	std::map<std::string, std::unique_ptr<TriviaGame>> games;
	// <guild_id, v8 instance>
	std::map<std::string, std::unique_ptr<V8Instance>> v8_instances;

	std::unique_ptr<boost::thread> heartbeat_thread;
};

#endif