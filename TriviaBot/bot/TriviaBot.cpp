#include <curl/curl.h>
#include <include/libplatform/libplatform.h>
#include <include/v8.h>

#include "ClientConnection.hpp"

std::string bot_token;

int main(int argc, char *argv[]) {
	curl_global_init(CURL_GLOBAL_DEFAULT);

	v8::V8::InitializeICUDefaultLocation(argv[0]);
	v8::V8::InitializeExternalStartupData(argv[0]);
	v8::Platform* platform = v8::platform::CreateDefaultPlatform();
	v8::V8::InitializePlatform(platform);
	v8::V8::Initialize();

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

	v8::V8::Dispose();
	v8::V8::ShutdownPlatform();
	delete platform;

	curl_global_cleanup();

	return 0;
}