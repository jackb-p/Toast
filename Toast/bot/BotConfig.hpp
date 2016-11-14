#ifndef BOT_BOTCONFIG
#define BOT_BOTCONFIG

#include <string>
#include <unordered_set>

class BotConfig {
public:
	BotConfig();

	bool is_new_config;

	std::string token;
	std::string owner_id;
	std::string cert_location;
	std::unordered_set<std::string> js_allowed_roles;

private:
	void load_from_json(std::string data);
	void create_new_file();
};

#endif
