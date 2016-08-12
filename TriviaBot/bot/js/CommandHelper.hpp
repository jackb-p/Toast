#ifndef BOT_JS_COMMANDHELPER
#define BOT_JS_COMMANDHELPER

#include <string>
#include <vector>

namespace CommandHelper {
	struct Command {
		std::string guild_id;
		std::string command_name;
		std::string script;
	};

	void init();
	int insert_command(std::string guild_id, std::string command_name, std::string script);
	bool get_command(std::string guild_id, std::string name, Command &command);
	bool command_in_db(std::string guild_id, std::string command_name);
}

#endif
