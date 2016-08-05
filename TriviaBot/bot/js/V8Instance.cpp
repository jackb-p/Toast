#include <iostream>
#include <string>

#include "V8Instance.hpp"
#include "../APIHelper.hpp"
#include "../Logger.hpp"

V8Instance::V8Instance(std::string guild_id, std::shared_ptr<APIHelper> ah, std::map<std::string, DiscordObjects::Guild> *guilds, std::map<std::string, DiscordObjects::Channel> *channels,
	std::map<std::string, DiscordObjects::User> *users, std::map<std::string, DiscordObjects::Role> *roles) {

	this->guild_id = guild_id;
	this->ah = ah;
	this->guilds = guilds;
	this->channels = channels;
	this->users = users;
	this->roles = roles;

	create();
}

V8Instance::~V8Instance() {
	clean_up();
}

void V8Instance::create() {
	Isolate::CreateParams create_params;
	create_params.array_buffer_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();

	isolate = Isolate::New(create_params);
	isolate->Enter();
	Logger::write("[v8] Created isolate", Logger::LogLevel::Debug);

	Isolate::Scope isolate_scope(isolate);
	HandleScope handle_scope(isolate);

	// set global context
	Local<Context> context = create_context();
	context->Enter();
	Context::Scope context_scope(context);
	Logger::write("[v8] Created context and context scope", Logger::LogLevel::Debug);
}

v8::Local<v8::Context> V8Instance::create_context() {
	Local<ObjectTemplate> global = ObjectTemplate::New(isolate);
	// bind print() function
	Local<External> self = External::New(isolate, (void *) this);

	global->Set(
		String::NewFromUtf8(isolate, "print", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, V8Instance::js_print, self)
	);
	global->SetAccessor(
		String::NewFromUtf8(isolate, "server", NewStringType::kNormal).ToLocalChecked(), 
		V8Instance::js_get_server, 
		(AccessorSetterCallback) 0, 
		self
	);
	global->SetAccessor(
		String::NewFromUtf8(isolate, "channel", NewStringType::kNormal).ToLocalChecked(), 
		V8Instance::js_get_channel, 
		(AccessorSetterCallback) 0, 
		self
	); 
	global->SetAccessor(
		String::NewFromUtf8(isolate, "user", NewStringType::kNormal).ToLocalChecked(), 
		V8Instance::js_get_user,
		(AccessorSetterCallback) 0, 
		self
	); 
	global->SetAccessor(
		String::NewFromUtf8(isolate, "input", NewStringType::kNormal).ToLocalChecked(), 
		V8Instance::js_get_input, 
		(AccessorSetterCallback) 0, 
		self
	);

	Logger::write("[v8] Created global obj, linked data and functions", Logger::LogLevel::Debug);

	return Context::New(isolate, NULL, global);
}

void V8Instance::js_get_server(Local<String> property, const PropertyCallbackInfo<Value> &info) {
	auto data = info.Data().As<External>();
	V8Instance *self = static_cast<V8Instance *>(data->Value());

	Local<Object> obj = Object::New(info.GetIsolate());
	self->add_to_obj(obj, (*self->guilds)[self->guild_id]);
	info.GetReturnValue().Set(obj);
}

void V8Instance::js_get_channel(Local<String> property, const PropertyCallbackInfo<Value> &info) {
	auto data = info.Data().As<External>();
	V8Instance *self = static_cast<V8Instance *>(data->Value());

	if (!self->current_channel) {
		Logger::write("[v8] current_channel is null pointer", Logger::LogLevel::Severe);
		info.GetReturnValue().SetNull();
		return;
	}

	Local<Object> obj = Object::New(info.GetIsolate());
	self->add_to_obj(obj, (*self->current_channel));
	info.GetReturnValue().Set(obj);
}

void V8Instance::js_get_user(Local<String> property, const PropertyCallbackInfo<Value> &info) {
	auto data = info.Data().As<External>();
	V8Instance *self = static_cast<V8Instance *>(data->Value());

	if (!self->current_sender) {
		Logger::write("[v8] current_sender is null pointer", Logger::LogLevel::Severe);
		info.GetReturnValue().SetNull();
		return;
	}

	Local<Object> obj = Object::New(info.GetIsolate());
	self->add_to_obj(obj, (*self->current_sender));
	info.GetReturnValue().Set(obj);
}

void V8Instance::js_get_input(Local<String> property, const PropertyCallbackInfo<Value> &info) {
	auto data = info.Data().As<External>();
	V8Instance *self = static_cast<V8Instance *>(data->Value());

	info.GetReturnValue().Set(String::NewFromUtf8(info.GetIsolate(), self->current_input.c_str(), NewStringType::kNormal).ToLocalChecked());
}

void V8Instance::clean_up() {
	Logger::write("[v8] Cleaning up", Logger::LogLevel::Debug);
	isolate->Exit();
	isolate->Dispose();
}

void V8Instance::reload() {
	clean_up();
	create();
}

void V8Instance::exec_js(std::string js, DiscordObjects::Channel *channel, DiscordObjects::GuildMember *sender, std::string args) {
	HandleScope handle_scope(isolate);
	Local<Context> context(isolate->GetCurrentContext());

	current_sender = sender;
	current_channel = channel;
	current_input = args;

	Logger::write("[v8] Preparing JS: " + js, Logger::LogLevel::Debug);

	Local<String> source = String::NewFromUtf8(isolate, js.c_str(), NewStringType::kNormal).ToLocalChecked();

	// compile
	Logger::write("[v8] Isolate nullptr? " + std::to_string(isolate == nullptr) + " Context empty? " + std::to_string(context.IsEmpty()), Logger::LogLevel::Debug);

	TryCatch compile_try_catch(isolate);
	Local<Script> script;

	if (!Script::Compile(context, source).ToLocal(&script)) {
		String::Utf8Value error(compile_try_catch.Exception());

		std::string err_msg = *error;
		Logger::write("[v8] Compilation error: " + err_msg, Logger::LogLevel::Debug);
		ah->send_message(channel->id, ":warning: **Compilation error:** `" + err_msg + "`");

		return;
	}

	TryCatch run_try_catch(isolate);
	MaybeLocal<Value> v = script->Run(context);
	if (v.IsEmpty()) {
		String::Utf8Value error(run_try_catch.Exception());

		std::string err_msg = *error;
		Logger::write("[v8] Runtime error: " + err_msg, Logger::LogLevel::Debug);
		ah->send_message(channel->id, ":warning: **Runtime error:** `" + err_msg + "`");
	}

	Logger::write("[v8] Script compiled and run", Logger::LogLevel::Debug);

	current_sender = nullptr;
	current_channel = nullptr;
	current_input = "";

	if (print_text != "") {
		ah->send_message(channel->id, print_text);
		print_text = "";
	}
}

void V8Instance::js_print(const v8::FunctionCallbackInfo<v8::Value> &args) {
	auto data = args.Data().As<External>();
	V8Instance *self = static_cast<V8Instance *>(data->Value());

	std::string output = "";
	for (int i = 0; i < args.Length(); i++) {
		v8::HandleScope handle_scope(args.GetIsolate());
		v8::String::Utf8Value str(args[i]);
		self->print_text += *str;
	}
}

void V8Instance::add_to_obj(Local<Object> &object, std::string field_name, std::string value) {
	add_to_obj(object, field_name, value.c_str());
}

void V8Instance::add_to_obj(Local<Object> &object, std::string field_name, const char value[]) {
	if (value == "null") {
		object->Set(String::NewFromUtf8(isolate, field_name.c_str(), NewStringType::kNormal).ToLocalChecked(), Null(isolate));
		return;
	}

	object->Set(String::NewFromUtf8(isolate, field_name.c_str(), NewStringType::kNormal).ToLocalChecked(),
		String::NewFromUtf8(isolate, value, NewStringType::kNormal).ToLocalChecked());
}

void V8Instance::add_to_obj(Local<Object> &object, std::string field_name, int32_t value) {
	object->Set(String::NewFromUtf8(isolate, field_name.c_str(), NewStringType::kNormal).ToLocalChecked(),
		Integer::New(isolate, value));
}

void V8Instance::add_to_obj(Local<Object> &object, std::string field_name, bool value) {
	object->Set(String::NewFromUtf8(isolate, field_name.c_str(), NewStringType::kNormal).ToLocalChecked(),
		Boolean::New(isolate, value));
}

void V8Instance::add_to_obj(Local<Object> &object, std::string field_name, Local<Object> value) {
	object->Set(String::NewFromUtf8(isolate, field_name.c_str(), NewStringType::kNormal).ToLocalChecked(), value);
}

void V8Instance::add_to_obj(Local<Object> &object, std::string field_name, Local<Array> value) {
	object->Set(String::NewFromUtf8(isolate, field_name.c_str(), NewStringType::kNormal).ToLocalChecked(), value);
}

void V8Instance::add_to_obj(Local<Object> &object, DiscordObjects::Guild guild) {
	/* Boobot fields */
	add_to_obj(object, "Id", guild.id);
	add_to_obj(object, "Name", guild.name);
	add_to_obj(object, "IconUrl", "https://discordapp.com/api/guilds/" + guild.id + "/icons/" + guild.icon + ".jpg");

	Local<Object> owner_obj = Object::New(isolate);
	DiscordObjects::GuildMember &owner = guild.members[guild.owner_id];
	add_to_obj(owner_obj, owner);
	add_to_obj(object, "Owner", owner_obj);

	Local<Array> roles_arr = Array::New(isolate, guild.roles.size());
	for (uint32_t i = 0; i < guild.roles.size(); i++) {
		Local<Object> obj = Object::New(isolate);
		DiscordObjects::Role &role = *guild.roles[i];
		add_to_obj(obj, role);

		roles_arr->Set(i, obj);
	}
	add_to_obj(object, "Roles", roles_arr);

	Local<Array> members_arr = Array::New(isolate, guild.members.size());
	int i = 0;
	for (auto it : guild.members) {
		Local<Object> obj = Object::New(isolate);
		DiscordObjects::GuildMember &member = it.second;
		add_to_obj(obj, member);

		members_arr->Set(i, obj);
		i++;
	}
	add_to_obj(object, "Users", members_arr);
}

void V8Instance::add_to_obj(Local<Object> &object, DiscordObjects::Channel channel) {
	/* Boobot fields */
	add_to_obj(object, "Id", channel.id);
	add_to_obj(object, "Name", channel.name);
	add_to_obj(object, "Topic", channel.topic);
	add_to_obj(object, "IsVoice", channel.type == "voice");
	
	Local<Array> users = Array::New(isolate, 1);
	users->Set(0, String::NewFromUtf8(isolate, "NOT IMPLEMENTED", NewStringType::kNormal).ToLocalChecked());
	add_to_obj(object, "Users", users);

	/* Additional fields */
	add_to_obj(object, "LastMessageId", channel.last_message_id);
	add_to_obj(object, "Bitrate", channel.bitrate);
	add_to_obj(object, "UserLimit", channel.user_limit);
}

void V8Instance::add_to_obj(Local<Object> &object, DiscordObjects::Role role) {
	/* Boobot fields */
	add_to_obj(object, "Id", role.id);
	add_to_obj(object, "Name", role.name);
	add_to_obj(object, "Position", role.position);
	add_to_obj(object, "Red", "NOT IMPLEMENTED");
	add_to_obj(object, "Blue", "NOT IMPLEMENTED");
	add_to_obj(object, "Green", "NOT IMPLEMENTED");

	/* Additional fields */
	add_to_obj(object, "Mentionable", role.mentionable);
	add_to_obj(object, "Mention", "<@&" + role.id + ">");
	add_to_obj(object, "Hoist", role.hoist);
}

void V8Instance::add_to_obj(Local<Object> &object, DiscordObjects::GuildMember member) {
	/* Boobot fields */
	add_to_obj(object, "Id", member.user->id);
	add_to_obj(object, "Name", member.user->username);
	add_to_obj(object, "Mention", "<@!" + member.user->id + ">");
	add_to_obj(object, "AvatarUrl", "https://discordapp.com/api/users/" + member.user->id + "/avatars/" + member.user->avatar + ".jpg");

	Local<Array> roles = Array::New(isolate, member.roles.size());
	int i = 0;
	for (DiscordObjects::Role *role : member.roles) {
		Local<Object> role_obj = Object::New(isolate);
		add_to_obj(role_obj, *role);

		roles->Set(i, role_obj);
		i++;
	}
	add_to_obj(object, "Roles", roles);

	add_to_obj(object, "State", "NOT IMPLEMENTED");
	add_to_obj(object, "CurrentGame", "NOT IMPLEMENTED");

	/* Additional fields */
	add_to_obj(object, "Nick", member.nick);
	add_to_obj(object, "Deaf", member.deaf);
	add_to_obj(object, "Mute", member.mute);
	add_to_obj(object, "JoinedAt", member.joined_at);
}