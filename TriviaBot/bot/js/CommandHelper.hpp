#ifndef BOT_JS_COMMANDHELPER
#define BOT_JS_COMMANDHELPER

#include <vector>

struct Command {
	std::string guild_id;
	std::string command_name;
	std::string script;
};

class CommandHelper {
public:
	CommandHelper();
	int insert_command(std::string guild_id, std::string command_name, std::string script);
	bool get_command(std::string guild_id, std::string name, Command &command);

private:
	bool command_in_db(std::string guild_id, std::string command_name);
	bool return_code_ok(int return_code);

	std::vector<Command> commands;
};

#endif