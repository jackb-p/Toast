#include <curl/curl.h>
#include <include/libplatform/libplatform.h>
#include <include/v8.h>

#include "ClientConnection.hpp"
#include "Logger.hpp"
#include "DiscordAPI.hpp"

std::string bot_token;

int main(int argc, char *argv[]) {
	curl_global_init(CURL_GLOBAL_DEFAULT);

	v8::V8::InitializeICUDefaultLocation(argv[0]);
	v8::V8::InitializeExternalStartupData(argv[0]);
	v8::Platform* platform = v8::platform::CreateDefaultPlatform();
	v8::V8::InitializePlatform(platform);
	v8::V8::Initialize();

	Logger::write("Initialised V8 and curl", Logger::LogLevel::Debug);

	if (argc == 2) {
		bot_token = argv[1];
	}
	else {
		std::cout << "Please enter your bot token: " << std::endl;
		std::cin >> bot_token;
	}

	std::string args = "/?v=5&encoding=json";
	std::string url = DiscordAPI::get_gateway().value("url", "wss://gateway.discord.gg");

	bool retry = true;
	while (retry) {
		try {
			ClientConnection conn;
			conn.start(url + args);
		}
		catch (const std::exception &e) {
			Logger::write("std exception: " + std::string(e.what()), Logger::LogLevel::Severe);
			retry = false;
		}
		catch (websocketpp::lib::error_code e) {
			Logger::write("websocketpp exception: " + e.message(), Logger::LogLevel::Severe);
		}
		catch (...) {
			Logger::write("other exception.", Logger::LogLevel::Severe);
			retry = false;
		}
	}

	v8::V8::Dispose();
	v8::V8::ShutdownPlatform();
	delete platform;

	curl_global_cleanup();

	Logger::write("Cleaned up", Logger::LogLevel::Info);

	std::cout << "Press enter to exit" << std::endl;
	std::getchar();

	return 0;
}