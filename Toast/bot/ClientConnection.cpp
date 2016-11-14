#include "ClientConnection.hpp"

#include <cstdio>
#include <iostream>

#include "Logger.hpp"
#include "BotConfig.hpp"

ClientConnection::ClientConnection(BotConfig &c) : config(c), gh(config) {
	// Reset the log channels
	cli.clear_access_channels(websocketpp::log::alevel::all);

	// Only want application logging, logging from the initial connection stages or any error logging
	cli.set_access_channels(websocketpp::log::alevel::app | websocketpp::log::alevel::connect);
	cli.set_error_channels(websocketpp::log::elevel::all);

	// Initialize ASIO
	cli.init_asio();

	// Bind handlers
	cli.set_socket_init_handler(bind(
		&ClientConnection::on_socket_init,
		this,
		websocketpp::lib::placeholders::_1
	));
	cli.set_tls_init_handler(bind<context_ptr>(
		&ClientConnection::on_tls_init,
		this,
		websocketpp::lib::placeholders::_1
		));
	cli.set_message_handler(bind(
		&ClientConnection::on_message,
		this,
		websocketpp::lib::placeholders::_1,
		websocketpp::lib::placeholders::_2
	));
	cli.set_open_handler(bind(
		&ClientConnection::on_open,
		this,
		websocketpp::lib::placeholders::_1
	));
	cli.set_close_handler(bind(
		&ClientConnection::on_close,
		this,
		websocketpp::lib::placeholders::_1
	));
	cli.set_fail_handler(bind(
		&ClientConnection::on_fail,
		this,
		websocketpp::lib::placeholders::_1
	));
}

// Open a connection to the URI provided
void ClientConnection::start(std::string uri) {
	websocketpp::lib::error_code ec;
	client::connection_ptr con = cli.get_connection(uri, ec);

	if (ec) { // failed to create connection
		Logger::write("Failed to create connection: " + ec.message(), Logger::LogLevel::Severe);
		return;
	}

	// Open the connection
	cli.connect(con);
	cli.run();

	Logger::write("Finished running", Logger::LogLevel::Debug);
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
	client::connection_ptr con = cli.get_con_from_hdl(hdl);

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
	gh.handle_data(message->get_payload(), cli, hdl);
}

void ClientConnection::on_close(websocketpp::connection_hdl) {
	Logger::write("Connection closed", Logger::LogLevel::Info);
	cli.stop();
}