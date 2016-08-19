#include "BotConfig.hpp"

#include <sstream>
#include <fstream>
#include <ostream>

#include "json/json.hpp"

#include "Logger.hpp"

using json = nlohmann::json;

BotConfig::BotConfig() {
	is_new_config = false;
	std::stringstream ss;

	std::ifstream config_file("config.json");
	if(!config_file) {
		config_file.close();
		create_new_file();
		return;
	}

	ss << config_file.rdbuf();
	config_file.close();
	std::string config = ss.str();
	load_from_json(config);
}

void BotConfig::load_from_json(std::string data) {
	json parsed = json::parse(data);

	token = parsed.value("bot_token", "");
	owner_id = parsed.value("owner_id", "");
	cert_location = parsed.value("api_cert_file", "bot/http/DiscordCA.crt");

	js_allowed_roles = parsed["v8"].value("js_allowed_roles", std::unordered_set<std::string> { "Admin", "Coder" });

	Logger::write("config.json file loaded", Logger::LogLevel::Info);
}

void BotConfig::create_new_file() {
	std::string config = json {
		{ "bot_token", "" },
		{ "owner_id", "" },
		{ "api_cert_file", "bot/http/DiscordCA.crt" },
		{ "v8", {
			{ "js_allowed_roles", {
				"Admin", "Coder", "Bot Commander"
			} }
		} }
	}.dump(4);

	std::ofstream config_file("config.json");
	config_file << config;
	config_file.close();

	Logger::write("Created new config.json file", Logger::LogLevel::Info);
	is_new_config = true;
}