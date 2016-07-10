#include <curl/curl.h>

#include "ClientConnection.hpp"

int main() {
	curl_global_init(CURL_GLOBAL_DEFAULT);

	std::string uri = "wss://gateway.discord.gg/?v=5&encoding=json";

	try {
		ClientConnection endpoint;
		endpoint.start(uri);
	}
	catch (const std::exception & e) {
		std::cout << e.what() << std::endl;
	}
	catch (websocketpp::lib::error_code e) {
		std::cout << e.message() << std::endl;
	}
	catch (...) {
		std::cout << "other exception" << std::endl;
	}

	std::getchar();

	curl_global_cleanup();

	return 0;
}