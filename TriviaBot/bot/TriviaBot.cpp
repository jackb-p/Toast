#include <curl/curl.h>

#include "ClientConnection.hpp"

std::string bot_token;

int main(int argc, char *argv[]) {
	curl_global_init(CURL_GLOBAL_DEFAULT);

	if (argc == 2) {
		bot_token = argv[1];
	}
	else {
		std::cout << "Please enter your bot token: " << std::endl;
		std::cin >> bot_token;
	}

	// todo: get this using API
	std::string uri = "wss://gateway.discord.gg/?v=5&encoding=json";

	try {
		ClientConnection conn;
		conn.start(uri);
	}
	catch (const std::exception & e) {
		std::cerr << e.what() << std::endl;
	}
	catch (websocketpp::lib::error_code e) {
		std::cerr << e.message() << std::endl;
	}
	catch (...) {
		std::cerr << "other exception" << std::endl;
	}

	std::getchar();

	curl_global_cleanup();

	return 0;
}