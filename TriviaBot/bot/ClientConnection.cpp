#include "ClientConnection.hpp"

#include <cstdio>
#include <iostream>

#include "GatewayHandler.hpp"
#include "Logger.hpp"

ClientConnection::ClientConnection() {
	// Reset the log channels
	c.clear_access_channels(websocketpp::log::alevel::all);

	// Only want application logging, logging from the initial connection stages or any error logging
	c.set_access_channels(websocketpp::log::alevel::app | websocketpp::log::alevel::connect);
	c.set_error_channels(websocketpp::log::elevel::all);

	// Initialize ASIO
	c.init_asio();

	// Bind handlers
	c.set_socket_init_handler(bind(
		&ClientConnection::on_socket_init,
		this,
		websocketpp::lib::placeholders::_1
	));
	c.set_tls_init_handler(bind<context_ptr>(
		&ClientConnection::on_tls_init,
		this,
		websocketpp::lib::placeholders::_1
		));
	c.set_message_handler(bind(
		&ClientConnection::on_message,
		this,
		websocketpp::lib::placeholders::_1,
		websocketpp::lib::placeholders::_2
	));
	c.set_open_handler(bind(
		&ClientConnection::on_open,
		this,
		websocketpp::lib::placeholders::_1
	));
	c.set_close_handler(bind(
		&ClientConnection::on_close,
		this,
		websocketpp::lib::placeholders::_1
	));
	c.set_fail_handler(bind(
		&ClientConnection::on_fail,
		this,
		websocketpp::lib::placeholders::_1
	));

	gh = std::make_unique<GatewayHandler>();
}

// Open a connection to the URI provided
void ClientConnection::start(std::string uri) {
	websocketpp::lib::error_code ec;
	client::connection_ptr con = c.get_connection(uri, ec);

	if (ec) { // failed to create connection
		Logger::write("Failed to create connection: " + ec.message(), Logger::LogLevel::Severe);
		return;
	}

	// Open the connection
	c.connect(con);
	c.run();
}

// Event handlers
void ClientConnection::on_socket_init(websocketpp::connection_hdl) {
	Logger::write("Socket initialised", Logger::LogLevel::Debug);
}

context_ptr ClientConnection::on_tls_init(websocketpp::connection_hdl) {
	context_ptr ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

	try {
		ctx->set_options(boost::asio::ssl::context::default_workarounds |
			boost::asio::ssl::context::no_sslv2 |
			boost::asio::ssl::context::no_sslv3 |
			boost::asio::ssl::context::single_dh_use);
	}
	catch (std::exception &e) {
		Logger::write("[tls] Error in context pointer: " + std::string(e.what()), Logger::LogLevel::Severe);
	}
	return ctx;
}

void ClientConnection::on_fail(websocketpp::connection_hdl hdl) {
	client::connection_ptr con = c.get_con_from_hdl(hdl);

	// Print as much information as possible
	Logger::write("Fail handler: \n" +
		std::to_string(con->get_state()) + "\n" +
		std::to_string(con->get_local_close_code()) + "\n" +
		con->get_local_close_reason() + "\n" +
		std::to_string(con->get_remote_close_code()) + "\n" +
		con->get_remote_close_reason() + "\n" +
		std::to_string(con->get_ec().value()) + " - " + con->get_ec().message() + "\n",
		Logger::LogLevel::Severe);
}

void ClientConnection::on_open(websocketpp::connection_hdl hdl) {
	Logger::write("Connection opened", Logger::LogLevel::Debug);
}

void ClientConnection::on_message(websocketpp::connection_hdl hdl, message_ptr message) {
	if (message->get_opcode() != websocketpp::frame::opcode::text) {
		// If the message is not text, just print as hex
		Logger::write("Non-text message received: " + websocketpp::utility::to_hex(message->get_payload()), Logger::LogLevel::Warning);
		return;
	}


	// Pass the message to the gateway handler
	gh->handle_data(message->get_payload(), c, hdl);
}

void ClientConnection::on_close(websocketpp::connection_hdl) {
	Logger::write("Connection closed", Logger::LogLevel::Info);
}