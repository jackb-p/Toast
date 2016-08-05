#ifndef BOT_JS_V8INSTANCE
#define BOT_JS_V8INSTANCE

#include <memory>
#include <map>

#include <include/v8.h>
#include <include/libplatform/libplatform.h>

#include "../data_structures/Guild.hpp"
#include "../data_structures/Channel.hpp"
#include "../data_structures/Role.hpp"
#include "../data_structures/GuildMember.hpp"
#include "../data_structures/User.hpp"

class APIHelper;

using namespace v8;

class V8Instance {
public:
	V8Instance(std::string guild_id, std::shared_ptr<APIHelper> ah, std::map<std::string, DiscordObjects::Guild> *guilds,
		std::map<std::string, DiscordObjects::Channel> *channels, std::map<std::string, DiscordObjects::User> *users, std::map<std::string, DiscordObjects::Role> *roles);
	~V8Instance();
	void reload();
	void exec_js(std::string js, DiscordObjects::Channel *channel, DiscordObjects::GuildMember *sender, std::string args = "");

private:
	void clean_up();
	void create();
	Local<Context> create_context();

	void add_to_obj(Local<Object> &object, std::string field_name, std::string value);
	void add_to_obj(Local<Object> &object, std::string field_name, const char value[]);
	void add_to_obj(Local<Object> &object, std::string field_name, int32_t value);
	void add_to_obj(Local<Object> &object, std::string field_name, bool value);
	void add_to_obj(Local<Object> &object, std::string field_name, Local<Object> value);
	void add_to_obj(Local<Object> &object, std::string field_name, Local<Array> value);

	void add_to_obj(Local<Object> &object, DiscordObjects::Guild guild);
	void add_to_obj(Local<Object> &object, DiscordObjects::Channel channel);
	void add_to_obj(Local<Object> &object, DiscordObjects::Role role);
	void add_to_obj(Local<Object> &object, DiscordObjects::GuildMember member);

	static void js_print(const FunctionCallbackInfo<Value> &args);
	static void js_get_server(Local<String> property, const PropertyCallbackInfo<Value> &info);
	static void js_get_channel(Local<String> property, const PropertyCallbackInfo<Value> &info);
	static void js_get_user(Local<String> property, const PropertyCallbackInfo<Value> &info);
	static void js_get_input(Local<String> property, const PropertyCallbackInfo<Value> &info);

	std::map<std::string, DiscordObjects::Guild> *guilds;
	std::map<std::string, DiscordObjects::Channel> *channels;
	std::map<std::string, DiscordObjects::User> *users;
	std::map<std::string, DiscordObjects::Role> *roles;

	std::string guild_id;
	Isolate *isolate;
	std::shared_ptr<APIHelper> ah;

	/* variables which change when a new command is executed */
	std::string print_text;
	std::string current_input;
	DiscordObjects::Channel *current_channel;
	DiscordObjects::GuildMember *current_sender;
};

#endif