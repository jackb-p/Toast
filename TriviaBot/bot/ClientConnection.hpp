#include <websocketpp/config/asio_client.hpp>

#include "json/json.hpp"

#include <websocketpp/client.hpp>

#include <iostream>

#include "ProtocolHandler.h"

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;

using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
typedef client::connection_ptr connection_ptr;

class ClientConnection {
public:
	typedef ClientConnection type;

	ClientConnection() {
		m_endpoint.clear_access_channels(websocketpp::log::alevel::all);

		m_endpoint.set_access_channels(websocketpp::log::alevel::app | websocketpp::log::alevel::connect);
		m_endpoint.set_error_channels(websocketpp::log::elevel::all);

		// Initialize ASIO
		m_endpoint.init_asio();

		// Register our handlers
		m_endpoint.set_socket_init_handler(bind(
			&type::on_socket_init,
			this,
			websocketpp::lib::placeholders::_1
		));
		m_endpoint.set_tls_init_handler(bind<context_ptr>(
			&type::on_tls_init,
			this,
			websocketpp::lib::placeholders::_1
		));
		m_endpoint.set_message_handler(bind(
			&type::on_message,
			this,
			websocketpp::lib::placeholders::_1,
			websocketpp::lib::placeholders::_2
		));
		m_endpoint.set_open_handler(bind(
			&type::on_open,
			this,
			websocketpp::lib::placeholders::_1
		));
		m_endpoint.set_close_handler(bind(
			&type::on_close,
			this,
			websocketpp::lib::placeholders::_1
		));
		m_endpoint.set_fail_handler(bind(
			&type::on_fail,
			this,
			websocketpp::lib::placeholders::_1
		));
	}

	void start(std::string uri) {
		websocketpp::lib::error_code ec;
		client::connection_ptr con = m_endpoint.get_connection(uri, ec);

		if (ec) {
			m_endpoint.get_alog().write(websocketpp::log::alevel::app, ec.message());
			return;
		}

		m_endpoint.connect(con);

		m_endpoint.run();
	}

	void on_socket_init(websocketpp::connection_hdl) {
		m_endpoint.get_alog().write(websocketpp::log::alevel::app,
			"Socket intialised.");
	}

	context_ptr on_tls_init(websocketpp::connection_hdl) {
		context_ptr ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

		try {
			ctx->set_options(boost::asio::ssl::context::default_workarounds |
				boost::asio::ssl::context::no_sslv2 |
				boost::asio::ssl::context::no_sslv3 |
				boost::asio::ssl::context::single_dh_use);
		}
		catch (std::exception& e) {
			std::cout << "Error in context pointer: " << e.what() << std::endl;
		}
		return ctx;
	}

	void on_fail(websocketpp::connection_hdl hdl) {
		client::connection_ptr con = m_endpoint.get_con_from_hdl(hdl);

		m_endpoint.get_elog().write(websocketpp::log::elevel::warn,
			"Fail handler: \n" +
			std::to_string(con->get_state()) + "\n" +
			std::to_string(con->get_local_close_code()) + "\n" +
			con->get_local_close_reason() + "\n" +
			std::to_string(con->get_remote_close_code()) + "\n" +
			con->get_remote_close_reason() + "\n" +
			std::to_string(con->get_ec().value()) + " - " + con->get_ec().message() + "\n");
	}

	void on_open(websocketpp::connection_hdl &hdl) {
		//ProtocolHandler::handle_data();
		//pHandler.handle_data("jjj", m_endpoint, hdl);
	}

	void on_message(websocketpp::connection_hdl &hdl, message_ptr message) {
		if (message->get_opcode() != websocketpp::frame::opcode::text) {
			std::cout << "<< " << websocketpp::utility::to_hex(message->get_payload()) << std::endl;
			return;
		}

		pHandler.handle_data(message->get_payload(), m_endpoint, hdl);
	}

	void on_close(websocketpp::connection_hdl) {
		std::cout << "Closed." << std::endl;
	}
private:
	client m_endpoint;
	ProtocolHandler pHandler;
};