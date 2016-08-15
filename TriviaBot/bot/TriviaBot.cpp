#include <thread>
#include <chrono>

#include <curl/curl.h>
#include <include/libplatform/libplatform.h>
#include <include/v8.h>

#include "ClientConnection.hpp"
#include "Logger.hpp"
#include "DiscordAPI.hpp"
#include "BotConfig.hpp"

int main(int argc, char *argv[]) {
	BotConfig config;
	if (config.is_new_config) {
		Logger::write("Since the config.json file is newly generated, the program will exit now to allow you to edit it.", Logger::LogLevel::Info);
		return 0;
	}

	curl_global_init(CURL_GLOBAL_DEFAULT);

	v8::V8::InitializeICUDefaultLocation(argv[0]);
	v8::V8::InitializeExternalStartupData(argv[0]);
	v8::Platform* platform = v8::platform::CreateDefaultPlatform();
	v8::V8::InitializePlatform(platform);
	v8::V8::Initialize();

	Logger::write("Initialised V8 and curl", Logger::LogLevel::Debug);

	std::string args = "/?v=5&encoding=json";
	std::string url = DiscordAPI::get_gateway(config.cert_location).value("url", "wss://gateway.discord.gg");

	bool retry = true;
	int exit_code = 0;
	while (retry) {
		retry = false;

		try {
			ClientConnection conn(config);
			conn.start(url + args);
		}
		catch (const std::exception &e) {
			Logger::write("std exception: " + std::string(e.what()), Logger::LogLevel::Severe);
			exit_code = 1;
		}
		catch (websocketpp::lib::error_code e) {
			Logger::write("websocketpp exception: " + e.message(), Logger::LogLevel::Severe);
			std::this_thread::sleep_for(std::chrono::seconds(10));
			retry = true; // should just be an occasional connection issue
		}
		catch (...) {
			Logger::write("other exception.", Logger::LogLevel::Severe);
			exit_code = 2;
		}
	}

	v8::V8::Dispose();
	v8::V8::ShutdownPlatform();
	delete platform;

	curl_global_cleanup();

	Logger::write("Cleaned up", Logger::LogLevel::Info);

	std::getchar();
	return exit_code;
}
