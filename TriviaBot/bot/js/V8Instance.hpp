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

class BotConfig;

class V8Instance {
public:
	V8Instance(BotConfig &c, std::string guild_id, std::map<std::string, DiscordObjects::Guild> *guilds,
		std::map<std::string, DiscordObjects::Channel> *channels, std::map<std::string, DiscordObjects::User> *users, std::map<std::string, DiscordObjects::Role> *roles);
	void exec_js(std::string js, DiscordObjects::Channel *channel, DiscordObjects::GuildMember *sender, std::string args = "");

private:
	BotConfig &config;

	void create();
	v8::Local<v8::Context> create_context();

	void initialise(v8::Local<v8::Context> context);

	/* server */
	v8::Global<v8::ObjectTemplate> server_template;
	v8::Local<v8::ObjectTemplate> make_server_template();
	v8::Local<v8::Object> wrap_server(DiscordObjects::Guild *guild);
	static void js_get_server(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value> &info);


	/* user */
	v8::Global<v8::ObjectTemplate> user_template;
	v8::Local<v8::ObjectTemplate> make_user_template();
	v8::Local<v8::Object> wrap_user(DiscordObjects::GuildMember *member);
	static void js_get_user(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value> &info);

	v8::Global<v8::ObjectTemplate> user_list_template;
	v8::Local<v8::ObjectTemplate> make_user_list_template();
	v8::Local<v8::Object> wrap_user_list(std::vector<DiscordObjects::GuildMember *> *user_list);
	static void js_get_user_list(uint32_t index, const v8::PropertyCallbackInfo<v8::Value> &info);

	/* channel */
	v8::Global<v8::ObjectTemplate> channel_template;
	v8::Local<v8::ObjectTemplate> make_channel_template();
	v8::Local<v8::Object> wrap_channel(DiscordObjects::Channel *channel);
	static void js_get_channel(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value> &info);

	v8::Global<v8::ObjectTemplate> channel_list_template;
	v8::Local<v8::ObjectTemplate> make_channel_list_template();
	v8::Local<v8::Object> wrap_channel_list(std::vector<DiscordObjects::Channel *> *channel_list);
	static void js_get_channel_list(uint32_t index, const v8::PropertyCallbackInfo<v8::Value> &info);

	/* role */
	v8::Global<v8::ObjectTemplate> role_template;
	v8::Local<v8::ObjectTemplate> make_role_template();
	v8::Local<v8::Object> wrap_role(DiscordObjects::Role *role);
	static void js_get_role(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value> &info);

	v8::Global<v8::ObjectTemplate> role_list_template;
	v8::Local<v8::ObjectTemplate> make_role_list_template();
	v8::Local<v8::Object> wrap_role_list(std::vector<DiscordObjects::Role *> *role_list);
	static void js_get_role_list(uint32_t index, const v8::PropertyCallbackInfo<v8::Value> &info);

	/* print function */
	static void js_print(const v8::FunctionCallbackInfo<v8::Value> &args);

	/* randomness functions */
	static void js_random(const v8::FunctionCallbackInfo<v8::Value> &args);
	static void js_shuffle(const v8::FunctionCallbackInfo<v8::Value> &args);

	std::map<std::string, DiscordObjects::Guild> *guilds;
	std::map<std::string, DiscordObjects::Channel> *channels;
	std::map<std::string, DiscordObjects::User> *users;
	std::map<std::string, DiscordObjects::Role> *roles;

	std::string guild_id;
	v8::Isolate *isolate;

	v8::Global<v8::Context> context_;

	/* random generating variables */
	std::mt19937 rng;

	/* variables which change when a new command is executed */
	std::string print_text;
	DiscordObjects::Channel *current_channel;
	DiscordObjects::GuildMember *current_sender;
};

#endif
