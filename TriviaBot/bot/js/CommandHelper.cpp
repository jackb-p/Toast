#include "CommandHelper.hpp"

#include <iostream>
#include <algorithm>

#include <sqlite3.h>

CommandHelper::CommandHelper() {
	sqlite3 *db; int return_code;
	return_code = sqlite3_open("bot/db/trivia.db", &db);

	std::string sql = "SELECT * FROM CustomJS";

	sqlite3_stmt *stmt;
	return_code = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);


	while (return_code != SQLITE_DONE) {
		return_code = sqlite3_step(stmt);

		if (return_code == SQLITE_ROW) {
			std::string guild_id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
			std::string command_name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
			std::string script = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));

			commands.push_back({ guild_id, command_name, script });
		}
		else if (return_code != SQLITE_DONE) {
			sqlite3_finalize(stmt);
			std::cerr << "SQLite error." << std::endl;
			return;
		}
	}

	std::cout << commands.size() << " commands loaded." << std::endl;

	sqlite3_finalize(stmt);
	sqlite3_close(db);
}

bool CommandHelper::get_command(std::string guild_id, std::string command_name, Command &command) {
	auto check_lambda = [guild_id, command_name](const Command &c) {
		return guild_id == c.guild_id && command_name == c.command_name;
	};

	auto it = std::find_if(commands.begin(), commands.end(), check_lambda);
	if (it == commands.end()) {
		command = {};
		return false;
	} else {
		command = { it->guild_id, it->command_name, it->script };
	}
}

// returns: 0 error, 1 inserted, 2 updated
int CommandHelper::insert_command(std::string guild_id, std::string command_name, std::string script) {
	// TODO: if script empty, delete command

	Command command { guild_id, command_name, script };

	int ret_value;
	std::string sql;
	if (command_in_db(guild_id, command_name)) {
		sql = "UPDATE CustomJS SET Script=?1 WHERE GuildID=?2 AND CommandName=?3;";
		std::cout << "Command already exists, updating." << std::endl;
		ret_value = 2;
	} else {
		sql = "INSERT INTO CustomJS(Script, GuildID, CommandName) VALUES (?1, ?2, ?3);";
		std::cout << "Inserting new command." << std::endl;
		ret_value = 1;
	}

	sqlite3 *db; int return_code;
	return_code = sqlite3_open("bot/db/trivia.db", &db);
	if (!return_code_ok(return_code)) return 0;

	sqlite3_stmt *stmt;
	return_code = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
	if (!return_code_ok(return_code)) return 0;

	return_code = sqlite3_bind_text(stmt, 1, script.c_str(), -1, (sqlite3_destructor_type)-1);
	if (!return_code_ok(return_code)) return 0;

	return_code = sqlite3_bind_text(stmt, 2, guild_id.c_str(), -1, (sqlite3_destructor_type)-1);
	if (!return_code_ok(return_code)) return 0;

	return_code = sqlite3_bind_text(stmt, 3, command_name.c_str(), -1, (sqlite3_destructor_type)-1);
	if (!return_code_ok(return_code)) return 0;

	return_code = sqlite3_step(stmt);
	bool success = return_code == SQLITE_DONE;

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	if (success) {
		if (ret_value == 1) {
			commands.push_back({ guild_id, command_name, script });
		}
		if (ret_value == 2) {
			// update command, don't add
			auto check_lambda = [guild_id, command_name](const Command &c) {
				return guild_id == c.guild_id && command_name == c.command_name;
			};

			auto it = std::find_if(commands.begin(), commands.end(), check_lambda);
			if (it == commands.end()) {
				return 0;
			} else {
				it->script = script;
			}
		}

		return ret_value;
	}

	return 0;
}

bool CommandHelper::command_in_db(std::string guild_id, std::string command_name) {
	sqlite3 *db; int return_code;
	return_code = sqlite3_open("bot/db/trivia.db", &db);
	if (!return_code_ok(return_code)) return false;

	std::string sql = "SELECT EXISTS(SELECT 1 FROM CustomJS WHERE GuildID=?1 AND CommandName=?2);";

	sqlite3_stmt *stmt;
	return_code = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
	if (!return_code_ok(return_code)) return false;

	return_code = sqlite3_bind_text(stmt, 1, guild_id.c_str(), -1, (sqlite3_destructor_type) -1);
	if (!return_code_ok(return_code)) return false;

	return_code = sqlite3_bind_text(stmt, 2, command_name.c_str(), -1, (sqlite3_destructor_type) -1);
	if (!return_code_ok(return_code)) return false;

	sqlite3_step(stmt);

	bool exists = sqlite3_column_int(stmt, 0) == 1; // returns 1 (true) if exists

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return exists;
}

bool CommandHelper::return_code_ok(int return_code) {
	if (return_code != SQLITE_OK) {
		std::cerr << "SQLite error. " << std::endl;
		return false;
	}
	return true;
}