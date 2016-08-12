#ifndef BOT_JS_V8INSTANCE
#define BOT_JS_V8INSTANCE

#include <memory>
#include <map>
#include <random>

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
	V8Instance(std::string guild_id, std::map<std::string, DiscordObjects::Guild> *guilds,
		std::map<std::string, DiscordObjects::Channel> *channels, std::map<std::string, DiscordObjects::User> *users, std::map<std::string, DiscordObjects::Role> *roles);
	~V8Instance();
	void reload();
	void exec_js(std::string js, DiscordObjects::Channel *channel, DiscordObjects::GuildMember *sender, std::string args = "");

private:
	void clean_up();
	void create();
	Local<Context> create_context();

	void initialise(Local<Context> context);

	/* server */
	Global<ObjectTemplate> server_template;
	Local<ObjectTemplate> make_server_template();
	Local<Object> wrap_server(DiscordObjects::Guild *guild);
	static void js_get_server(Local<Name> property, const PropertyCallbackInfo<Value> &info);


	/* user */
	Global<ObjectTemplate> user_template;
	Local<ObjectTemplate> make_user_template();
	Local<Object> wrap_user(DiscordObjects::GuildMember *member);
	static void js_get_user(Local<Name> property, const PropertyCallbackInfo<Value> &info);

	Global<ObjectTemplate> user_list_template;
	Local<ObjectTemplate> make_user_list_template();
	Local<Object> wrap_user_list(std::vector<DiscordObjects::GuildMember *> *user_list);
	static void js_get_user_list(uint32_t index, const PropertyCallbackInfo<Value> &info);

	/* channel */
	Global<ObjectTemplate> channel_template;
	Local<ObjectTemplate> make_channel_template();
	Local<Object> wrap_channel(DiscordObjects::Channel *channel);
	static void js_get_channel(Local<Name> property, const PropertyCallbackInfo<Value> &info);

	Global<ObjectTemplate> channel_list_template;
	Local<ObjectTemplate> make_channel_list_template();
	Local<Object> wrap_channel_list(std::vector<DiscordObjects::Channel *> *channel_list);
	static void js_get_channel_list(uint32_t index, const PropertyCallbackInfo<Value> &info);

	/* role */
	Global<ObjectTemplate> role_template;
	Local<ObjectTemplate> make_role_template();
	Local<Object> wrap_role(DiscordObjects::Role *role);
	static void js_get_role(Local<Name> property, const PropertyCallbackInfo<Value> &info);

	Global<ObjectTemplate> role_list_template;
	Local<ObjectTemplate> make_role_list_template();
	Local<Object> wrap_role_list(std::vector<DiscordObjects::Role *> *role_list);
	static void js_get_role_list(uint32_t index, const PropertyCallbackInfo<Value> &info);

	/* print function */
	static void js_print(const FunctionCallbackInfo<Value> &args);

	/* randomness functions */
	static void js_random(const FunctionCallbackInfo<Value> &args);
	static void js_shuffle(const FunctionCallbackInfo<Value> &args);

	std::map<std::string, DiscordObjects::Guild> *guilds;
	std::map<std::string, DiscordObjects::Channel> *channels;
	std::map<std::string, DiscordObjects::User> *users;
	std::map<std::string, DiscordObjects::Role> *roles;

	std::string guild_id;
	Isolate *isolate;

	Global<Context> context_;

	/* random generating variables */
	std::mt19937 rng;

	/* variables which change when a new command is executed */
	std::string print_text;
	DiscordObjects::Channel *current_channel;
	DiscordObjects::GuildMember *current_sender;
};

#endif
