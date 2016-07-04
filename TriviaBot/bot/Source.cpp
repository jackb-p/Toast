#include "ClientConnection.hpp"

int main(int argc, char* argv[]) {
	std::string uri = "wss://gateway.discord.gg/?v=5&encoding=json";
	//std::string uri = "wss://echo.websocket.org/";

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
}