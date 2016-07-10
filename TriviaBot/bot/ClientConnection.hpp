#ifndef BOT_CLIENTCONNECTION
#define BOT_CLIENTCONNECTION

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include "json/json.hpp"

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;

using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
typedef client::connection_ptr connection_ptr;

class GatewayHandler;

class ClientConnection {
public:
	ClientConnection();

	// Open a connection to the URI provided
	void start(std::string uri);

	// Event handlers
	void on_socket_init(websocketpp::connection_hdl);
	context_ptr on_tls_init(websocketpp::connection_hdl);
	void on_fail(websocketpp::connection_hdl hdl);
	void on_open(websocketpp::connection_hdl hdl);
	void on_message(websocketpp::connection_hdl &hdl, message_ptr message);
	void on_close(websocketpp::connection_hdl);

private:
	client c;
	GatewayHandler *gHandler;
};

#endif