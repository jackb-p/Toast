#include <iostream>
#include <string>
#include <chrono>
#include <algorithm>

#include "V8Instance.hpp"
#include "../DiscordAPI.hpp"
#include "../Logger.hpp"

V8Instance::V8Instance(std::string guild_id, std::map<std::string, DiscordObjects::Guild> *guilds, std::map<std::string, DiscordObjects::Channel> *channels,
	std::map<std::string, DiscordObjects::User> *users, std::map<std::string, DiscordObjects::Role> *roles) {

	rng = std::mt19937(std::random_device()());
	this->guild_id = guild_id;
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
	context_.Reset(isolate, context);
	Context::Scope context_scope(context);

	initialise(context);

	Logger::write("[v8] Created context and context scope", Logger::LogLevel::Debug);
}

void V8Instance::initialise(Local<Context> context) {
	HandleScope handle_scope(isolate);

	Local<Object> opts_obj = wrap_server(&(*guilds)[guild_id]);
	
	context->Global()->Set(
		context, 
		String::NewFromUtf8(isolate, "server", NewStringType::kNormal).ToLocalChecked(), 
		opts_obj
	).FromJust();

	Logger::write("[v8] Bound server template", Logger::LogLevel::Debug);
}

v8::Local<v8::Context> V8Instance::create_context() {
	Local<ObjectTemplate> global = ObjectTemplate::New(isolate);

	Local<External> self = External::New(isolate, (void *) this);
	global->Set(
		String::NewFromUtf8(isolate, "print", NewStringType::kNormal).ToLocalChecked(), 
		FunctionTemplate::New(isolate, V8Instance::js_print, self)
	);
	global->Set(
		String::NewFromUtf8(isolate, "random", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, V8Instance::js_random, self)
	);
	global->Set(
		String::NewFromUtf8(isolate, "shuffle", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, V8Instance::js_shuffle, self)
	);

	Logger::write("[v8] Created global context, added print function", Logger::LogLevel::Debug);

	return Context::New(isolate, NULL, global);
}

/* server */
Local<ObjectTemplate> V8Instance::make_server_template() {
	EscapableHandleScope handle_scope(isolate);

	Local<ObjectTemplate> templ = ObjectTemplate::New(isolate);
	templ->SetInternalFieldCount(1);
	templ->SetHandler(
		NamedPropertyHandlerConfiguration(
			V8Instance::js_get_server,
			(GenericNamedPropertySetterCallback) 0,
			(GenericNamedPropertyQueryCallback) 0,
			(GenericNamedPropertyDeleterCallback) 0,
			(GenericNamedPropertyEnumeratorCallback) 0,
			External::New(isolate, (void *) this)
		)
	);

	return handle_scope.Escape(templ);
}

Local<Object> V8Instance::wrap_server(DiscordObjects::Guild *guild) {
	EscapableHandleScope handle_scope(isolate);

	if (server_template.IsEmpty()) {
		Local<ObjectTemplate> raw_template = make_server_template();
		server_template.Reset(isolate, raw_template);
	}

	Local<ObjectTemplate> templ = Local<ObjectTemplate>::New(isolate, server_template);
	Local<Object> result = templ->NewInstance(isolate->GetCurrentContext()).ToLocalChecked();

	Local<External> guild_ptr = External::New(isolate, guild);
	result->SetInternalField(0, guild_ptr);

	return handle_scope.Escape(result);
}

void V8Instance::js_get_server(Local<Name> property, const PropertyCallbackInfo<Value> &info) {
	void *self_v = info.Data().As<External>()->Value();
	if (!self_v) {
		Logger::write("[v8] [js_get_server] Class pointer empty", Logger::LogLevel::Warning);
		return;
	}
	V8Instance *self = static_cast<V8Instance *>(self_v);

	void *guild_v = info.Holder()->GetInternalField(0).As<External>()->Value();
	if (!guild_v) {
		Logger::write("[v8] [js_get_server] Guild pointer empty", Logger::LogLevel::Warning);
		return;
	}
	DiscordObjects::Guild *guild = static_cast<DiscordObjects::Guild *>(guild_v);

	if (!property->IsString()) {
		return;
	}
	std::string property_s = *String::Utf8Value(property);

	if (property_s == "Id") {
		info.GetReturnValue().Set(String::NewFromUtf8(info.GetIsolate(), guild->id.c_str(), NewStringType::kNormal).ToLocalChecked());
	}
	else if (property_s == "Name") {
		info.GetReturnValue().Set(String::NewFromUtf8(info.GetIsolate(), guild->name.c_str(), NewStringType::kNormal).ToLocalChecked());
	}
	else if (property_s == "IconUrl") {
		std::string icon_url = "https://discordapp.com/api/guilds/" + guild->id + "/icons/" + guild->icon + ".jpg";
		info.GetReturnValue().Set(String::NewFromUtf8(info.GetIsolate(), icon_url.c_str(), NewStringType::kNormal).ToLocalChecked());
	}
	else if (property_s == "Owner") {
		std::string owner_id = guild->owner_id;
		DiscordObjects::GuildMember *owner = *std::find_if(guild->members.begin(), guild->members.end(), [owner_id](DiscordObjects::GuildMember *m) {
			return owner_id == m->user->id;
		});
		Local<Object> owner_obj = self->wrap_user(owner);
		info.GetReturnValue().Set(owner_obj);
	}
	else if (property_s == "Roles") {
		Local<Object> roles_obj = self->wrap_role_list(&guild->roles);
		info.GetReturnValue().Set(roles_obj);
	}
	else if (property_s == "Channels") {
		Local<Object> channels_obj = self->wrap_channel_list(&guild->channels);
		info.GetReturnValue().Set(channels_obj);
	}
	else if (property_s == "Users") {
		Local<Object> users_obj = self->wrap_user_list(&guild->members);
		info.GetReturnValue().Set(users_obj);
	}
}


/* channel */
Local<ObjectTemplate> V8Instance::make_channel_template() {
	EscapableHandleScope handle_scope(isolate);

	Local<ObjectTemplate> templ = ObjectTemplate::New(isolate);
	templ->SetInternalFieldCount(1);
	templ->SetHandler(
		NamedPropertyHandlerConfiguration(
			V8Instance::js_get_channel,
			(GenericNamedPropertySetterCallback) 0,
			(GenericNamedPropertyQueryCallback) 0,
			(GenericNamedPropertyDeleterCallback) 0,
			(GenericNamedPropertyEnumeratorCallback) 0,
			External::New(isolate, (void *) this)
		)
	);

	return handle_scope.Escape(templ);
}

Local<Object> V8Instance::wrap_channel(DiscordObjects::Channel *channel) {
	EscapableHandleScope handle_scope(isolate);

	if (role_template.IsEmpty()) {
		Local<ObjectTemplate> raw_template = make_channel_template();
		channel_template.Reset(isolate, raw_template);
	}

	Local<ObjectTemplate> templ = Local<ObjectTemplate>::New(isolate, channel_template);
	Local<Object> result = templ->NewInstance(isolate->GetCurrentContext()).ToLocalChecked();

	Local<External> channel_ptr = External::New(isolate, channel);
	result->SetInternalField(0, channel_ptr);

	return handle_scope.Escape(result);
}

void V8Instance::js_get_channel(Local<Name> property, const PropertyCallbackInfo<Value> &info) {
	void *self_v = info.Data().As<External>()->Value();
	if (!self_v) {
		Logger::write("[v8] [js_get_channel] Class pointer empty", Logger::LogLevel::Warning);
		return;
	}
	V8Instance *self = static_cast<V8Instance *>(self_v);

	void *channel_v = info.Holder()->GetInternalField(0).As<External>()->Value();
	if (!channel_v) {
		Logger::write("[v8] [js_get_channel] Channel pointer empty", Logger::LogLevel::Warning);
		return;
	}
	DiscordObjects::Channel *channel = static_cast<DiscordObjects::Channel *>(channel_v);

	if (!property->IsString()) {
		return;
	}
	std::string property_s = *String::Utf8Value(property);

	if (property_s == "Id") {
		info.GetReturnValue().Set(String::NewFromUtf8(info.GetIsolate(), channel->id.c_str(), NewStringType::kNormal).ToLocalChecked());
	}
	else if (property_s == "Name") {
		info.GetReturnValue().Set(String::NewFromUtf8(info.GetIsolate(), channel->name.c_str(), NewStringType::kNormal).ToLocalChecked());
	}
	else if (property_s == "Topic") {
		info.GetReturnValue().Set(String::NewFromUtf8(info.GetIsolate(), channel->topic.c_str(), NewStringType::kNormal).ToLocalChecked());
	}
	else if (property_s == "IsVoice") {
		info.GetReturnValue().Set(Boolean::New(info.GetIsolate(), channel->type == "voice"));
	}
	else if (property_s == "Users") {
		info.GetIsolate()->ThrowException(String::NewFromUtf8(info.GetIsolate(), "Channel.Users not implemented.", NewStringType::kNormal).ToLocalChecked());
	}
}

/* channel list */
Local<ObjectTemplate> V8Instance::make_channel_list_template() {
	EscapableHandleScope handle_scope(isolate);

	Local<ObjectTemplate> templ = ObjectTemplate::New(isolate);
	templ->SetInternalFieldCount(1);
	templ->SetHandler(
		IndexedPropertyHandlerConfiguration(
			V8Instance::js_get_channel_list,
			(IndexedPropertySetterCallback) 0,
			(IndexedPropertyQueryCallback) 0,
			(IndexedPropertyDeleterCallback) 0,
			(IndexedPropertyEnumeratorCallback) 0,
			External::New(isolate, (void *) this)
		)
	);

	return handle_scope.Escape(templ);
}

Local<Object> V8Instance::wrap_channel_list(std::vector<DiscordObjects::Channel *> *channel_list) {
	EscapableHandleScope handle_scope(isolate);

	if (channel_list_template.IsEmpty()) {
		Local<ObjectTemplate> raw_template = make_channel_list_template();
		channel_list_template.Reset(isolate, raw_template);
	}

	Local<ObjectTemplate> templ = Local<ObjectTemplate>::New(isolate, channel_list_template);
	Local<Object> result = templ->NewInstance(isolate->GetCurrentContext()).ToLocalChecked();

	// imitate an array
	result->Set(String::NewFromUtf8(isolate, "length", NewStringType::kNormal).ToLocalChecked(), Integer::New(isolate, (*channel_list).size()));
	result->SetPrototype(Array::New(isolate)->GetPrototype());

	Local<External> channel_list_ptr = External::New(isolate, channel_list);
	result->SetInternalField(0, channel_list_ptr);

	return handle_scope.Escape(result);
}

void V8Instance::js_get_channel_list(uint32_t index, const PropertyCallbackInfo<Value> &info) {
	void *self_v = info.Data().As<External>()->Value();
	if (!self_v) {
		Logger::write("[v8] [js_get_channel_list] Class pointer empty", Logger::LogLevel::Warning);
		return;
	}
	V8Instance *self = static_cast<V8Instance *>(self_v);

	void *channel_list_v = info.Holder()->GetInternalField(0).As<External>()->Value();
	if (!channel_list_v) {
		Logger::write("[v8] [js_get_channel_list] Channel List pointer empty", Logger::LogLevel::Warning);
		return;
	}
	std::vector<DiscordObjects::Channel *> *channel_list = static_cast<std::vector<DiscordObjects::Channel *> *>(channel_list_v);


	if (index < (*channel_list).size()) {
		Local<Object> channel_obj = self->wrap_channel((*channel_list)[index]);
		info.GetReturnValue().Set(channel_obj);
	}
	else {
		info.GetReturnValue().SetUndefined();
	}
}


/* user */
Local<ObjectTemplate> V8Instance::make_user_template() {
	EscapableHandleScope handle_scope(isolate);

	Local<ObjectTemplate> templ = ObjectTemplate::New(isolate);
	templ->SetInternalFieldCount(1);
	templ->SetHandler(
		NamedPropertyHandlerConfiguration(
			V8Instance::js_get_user,
			(GenericNamedPropertySetterCallback)0,
			(GenericNamedPropertyQueryCallback)0,
			(GenericNamedPropertyDeleterCallback)0,
			(GenericNamedPropertyEnumeratorCallback)0,
			External::New(isolate, (void *) this)
		)
	);

	return handle_scope.Escape(templ);
}

Local<Object> V8Instance::wrap_user(DiscordObjects::GuildMember *member) {
	EscapableHandleScope handle_scope(isolate);

	if (user_template.IsEmpty()) {
		Local<ObjectTemplate> raw_template = make_user_template();
		user_template.Reset(isolate, raw_template);
	}

	Local<ObjectTemplate> templ = Local<ObjectTemplate>::New(isolate, user_template);
	Local<Object> result = templ->NewInstance(isolate->GetCurrentContext()).ToLocalChecked();

	Local<External> member_ptr = External::New(isolate, member);
	result->SetInternalField(0, member_ptr);

	return handle_scope.Escape(result);
}

void V8Instance::js_get_user(Local<Name> property, const PropertyCallbackInfo<Value> &info) {
	void *self_v = info.Data().As<External>()->Value();
	if (!self_v) {
		Logger::write("[v8] [js_get_user] Class pointer empty", Logger::LogLevel::Warning);
		return;
	}
	V8Instance *self = static_cast<V8Instance *>(self_v);

	void *member_v = info.Holder()->GetInternalField(0).As<External>()->Value();
	if (!member_v) {
		Logger::write("[v8] [js_get_user] GuildMember pointer empty", Logger::LogLevel::Warning);
		return;
	}
	DiscordObjects::GuildMember *member = static_cast<DiscordObjects::GuildMember *>(member_v);

	if (!property->IsString()) {
		return;
	}
	std::string property_s = *String::Utf8Value(property);

	if (property_s == "Id") {
		info.GetReturnValue().Set(String::NewFromUtf8(info.GetIsolate(), member->user->id.c_str(), NewStringType::kNormal).ToLocalChecked());
	}
	else if (property_s == "Name") {
		std::string name = member->nick == "null" ? member->user->username : member->nick;
		info.GetReturnValue().Set(String::NewFromUtf8(info.GetIsolate(), name.c_str(), NewStringType::kNormal).ToLocalChecked());
	}
	else if (property_s == "Mention") {
		std::string mention = "<@" + member->user->id + ">";
		info.GetReturnValue().Set(String::NewFromUtf8(info.GetIsolate(), mention.c_str(), NewStringType::kNormal).ToLocalChecked());
	}
	else if (property_s == "AvatarUrl") {
		std::string avatar_url = "https://discordapp.com/api/users/" + member->user->id + "/avatars/" + member->user->avatar + ".jpg";
		info.GetReturnValue().Set(String::NewFromUtf8(info.GetIsolate(), avatar_url.c_str(), NewStringType::kNormal).ToLocalChecked());
	}
	else if (property_s == "Roles") {
		Local<Object> roles_obj = self->wrap_role_list(&member->roles);
		info.GetReturnValue().Set(roles_obj);
	}
	else if (property_s == "State") {
		info.GetReturnValue().Set(String::NewFromUtf8(info.GetIsolate(), member->user->status.c_str(), NewStringType::kNormal).ToLocalChecked());
	}
	else if (property_s == "CurrentGame") {
		if (member->user->game == "null") {
			info.GetReturnValue().SetNull();
		} else {
			info.GetReturnValue().Set(String::NewFromUtf8(info.GetIsolate(), member->user->game.c_str(), NewStringType::kNormal).ToLocalChecked());
		}
	}
}

/* user list */
Local<ObjectTemplate> V8Instance::make_user_list_template() {
	EscapableHandleScope handle_scope(isolate);

	Local<ObjectTemplate> templ = ObjectTemplate::New(isolate);
	templ->SetInternalFieldCount(1);
	templ->SetHandler(
		IndexedPropertyHandlerConfiguration(
			V8Instance::js_get_user_list,
			(IndexedPropertySetterCallback) 0,
			(IndexedPropertyQueryCallback) 0,
			(IndexedPropertyDeleterCallback) 0,
			(IndexedPropertyEnumeratorCallback) 0,
			External::New(isolate, (void *) this)
		)
	);

	return handle_scope.Escape(templ);
}

Local<Object> V8Instance::wrap_user_list(std::vector<DiscordObjects::GuildMember *> *user_list) {
	EscapableHandleScope handle_scope(isolate);

	if (user_list_template.IsEmpty()) {
		Local<ObjectTemplate> raw_template = make_user_list_template();
		user_list_template.Reset(isolate, raw_template);
	}

	Local<ObjectTemplate> templ = Local<ObjectTemplate>::New(isolate, user_list_template);
	Local<Object> result = templ->NewInstance(isolate->GetCurrentContext()).ToLocalChecked();

	// imitate an array
	result->Set(String::NewFromUtf8(isolate, "length", NewStringType::kNormal).ToLocalChecked(), Integer::New(isolate, (*user_list).size()));
	result->SetPrototype(Array::New(isolate)->GetPrototype());

	Local<External> user_list_ptr = External::New(isolate, user_list);
	result->SetInternalField(0, user_list_ptr);

	return handle_scope.Escape(result);
}

void V8Instance::js_get_user_list(uint32_t index, const PropertyCallbackInfo<Value> &info) {
	void *self_v = info.Data().As<External>()->Value();
	if (!self_v) {
		Logger::write("[v8] [js_get_user_list] Class pointer empty", Logger::LogLevel::Warning);
		return;
	}
	V8Instance *self = static_cast<V8Instance *>(self_v);

	void *user_list_v = info.Holder()->GetInternalField(0).As<External>()->Value();
	if (!user_list_v) {
		Logger::write("[v8] [js_get_user_list] GuildMember List pointer empty", Logger::LogLevel::Warning);
		return;
	}
	std::vector<DiscordObjects::GuildMember *> *user_list = static_cast<std::vector<DiscordObjects::GuildMember *> *>(user_list_v);

	if (index < (*user_list).size()) {
		Local<Object> role_obj = self->wrap_user((*user_list)[index]);
		info.GetReturnValue().Set(role_obj);
	}
	else {
		info.GetReturnValue().SetUndefined();
	}
}


/* role */
Local<ObjectTemplate> V8Instance::make_role_template() {
	EscapableHandleScope handle_scope(isolate);

	Local<ObjectTemplate> templ = ObjectTemplate::New(isolate);
	templ->SetInternalFieldCount(1);
	templ->SetHandler(
		NamedPropertyHandlerConfiguration(
			V8Instance::js_get_role,
			(GenericNamedPropertySetterCallback)0,
			(GenericNamedPropertyQueryCallback)0,
			(GenericNamedPropertyDeleterCallback)0,
			(GenericNamedPropertyEnumeratorCallback)0,
			External::New(isolate, (void *) this)
		)
	);

	return handle_scope.Escape(templ);
}

Local<Object> V8Instance::wrap_role(DiscordObjects::Role *role) {
	EscapableHandleScope handle_scope(isolate);

	if (role_template.IsEmpty()) {
		Local<ObjectTemplate> raw_template = make_role_template();
		role_template.Reset(isolate, raw_template);
	}

	Local<ObjectTemplate> templ = Local<ObjectTemplate>::New(isolate, role_template);
	Local<Object> result = templ->NewInstance(isolate->GetCurrentContext()).ToLocalChecked();

	Local<External> role_ptr = External::New(isolate, role);
	result->SetInternalField(0, role_ptr);

	return handle_scope.Escape(result);
}

void V8Instance::js_get_role(Local<Name> property, const PropertyCallbackInfo<Value> &info) {
	void *role_v = info.Holder()->GetInternalField(0).As<External>()->Value();
	if (!role_v) {
		Logger::write("[v8] [js_get_role] Role pointer empty", Logger::LogLevel::Warning);
		return;
	}
	DiscordObjects::Role *role = static_cast<DiscordObjects::Role *>(role_v);

	if (!property->IsString()) {
		return;
	}
	std::string property_s = *String::Utf8Value(property);

	if (property_s == "Id") {
		info.GetReturnValue().Set(String::NewFromUtf8(info.GetIsolate(), role->id.c_str(), NewStringType::kNormal).ToLocalChecked());
	}
	else if (property_s == "Name") {
		info.GetReturnValue().Set(String::NewFromUtf8(info.GetIsolate(), role->name.c_str(), NewStringType::kNormal).ToLocalChecked());
	}
	else if (property_s == "Position") {
		info.GetReturnValue().Set(Integer::New(info.GetIsolate(), role->position));
	}
	else if (property_s == "Red" || property_s == "Green" || property_s == "Blue") {
		info.GetIsolate()->ThrowException(String::NewFromUtf8(info.GetIsolate(), "Role.[Colour] not implemented.", NewStringType::kNormal).ToLocalChecked());
	}
}

/* role list */
Local<ObjectTemplate> V8Instance::make_role_list_template() {
	EscapableHandleScope handle_scope(isolate);

	Local<ObjectTemplate> templ = ObjectTemplate::New(isolate);
	templ->SetInternalFieldCount(1);
	templ->SetHandler(
		IndexedPropertyHandlerConfiguration(
			V8Instance::js_get_role_list,
			(IndexedPropertySetterCallback) 0,
			(IndexedPropertyQueryCallback) 0,
			(IndexedPropertyDeleterCallback) 0,
			(IndexedPropertyEnumeratorCallback) 0,
			External::New(isolate, (void *) this)
		)
	);

	return handle_scope.Escape(templ);
}

Local<Object> V8Instance::wrap_role_list(std::vector<DiscordObjects::Role *> *role_list) {
	EscapableHandleScope handle_scope(isolate);

	if (role_list_template.IsEmpty()) {
		Local<ObjectTemplate> raw_template = make_role_list_template();
		role_list_template.Reset(isolate, raw_template);
	}

	Local<ObjectTemplate> templ = Local<ObjectTemplate>::New(isolate, role_list_template);
	Local<Object> result = templ->NewInstance(isolate->GetCurrentContext()).ToLocalChecked();

	// imitate an array
	result->Set(String::NewFromUtf8(isolate, "length", NewStringType::kNormal).ToLocalChecked(), Integer::New(isolate, (*role_list).size()));
	result->SetPrototype(Array::New(isolate)->GetPrototype());

	Local<External> role_list_ptr = External::New(isolate, role_list);
	result->SetInternalField(0, role_list_ptr);

	return handle_scope.Escape(result);
}

void V8Instance::js_get_role_list(uint32_t index, const PropertyCallbackInfo<Value> &info) {
	void *self_v = info.Data().As<External>()->Value();
	if (!self_v) {
		Logger::write("[v8] [js_get_role_list] Class pointer empty", Logger::LogLevel::Warning);
		return;
	}
	V8Instance *self = static_cast<V8Instance *>(self_v);

	void *role_list_v = info.Holder()->GetInternalField(0).As<External>()->Value();
	if (!role_list_v) {
		Logger::write("[v8] [js_get_role_list] Role List pointer empty", Logger::LogLevel::Warning);
		return;
	}
	std::vector<DiscordObjects::Role *> *role_list = static_cast<std::vector<DiscordObjects::Role *> *>(role_list_v);


	if (index < (*role_list).size()) {
		Local<Object> role_obj = self->wrap_role((*role_list)[index]);
		info.GetReturnValue().Set(role_obj);
	}
	else {
		info.GetReturnValue().SetUndefined();
	}
}


/* global functions */
void V8Instance::js_print(const v8::FunctionCallbackInfo<v8::Value> &args) {
	auto data = args.Data().As<External>();
	V8Instance *self = static_cast<V8Instance *>(data->Value());

	std::string output = "";
	for (int i = 0; i < args.Length(); i++) {
		v8::String::Utf8Value str(args[i]);
		self->print_text += *str;
	}
	self->print_text += "\n";
}

void V8Instance::js_random(const v8::FunctionCallbackInfo<v8::Value> &args) {
	auto data = args.Data().As<External>();
	V8Instance *self = static_cast<V8Instance *>(data->Value());

	int number_args = args.Length();

	if (number_args == 0) {
		std::uniform_real_distribution<double> dist(0, 1);
		double random_val = dist(self->rng);
		args.GetReturnValue().Set(Number::New(args.GetIsolate(), random_val));
	}
	else if (number_args == 1) {
		int64_t max = args[0]->IntegerValue();
		std::uniform_int_distribution<int> dist(0, max);
		int random_val = dist(self->rng);
		args.GetReturnValue().Set(Integer::New(args.GetIsolate(), random_val));
	}
	else if (number_args == 2) {
		int64_t min = args[0]->IntegerValue();
		int64_t max = args[1]->IntegerValue();
		std::uniform_int_distribution<int> dist(min, max);
		int random_val = dist(self->rng);
		args.GetReturnValue().Set(Integer::New(args.GetIsolate(), random_val));
	}
	else {
		std::string err_msg = "random() requires 0-2 arguments. You gave: " + std::to_string(number_args);
		args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), err_msg.c_str(), NewStringType::kNormal).ToLocalChecked());
	}
}

void V8Instance::js_shuffle(const v8::FunctionCallbackInfo<v8::Value> &args) {
	auto data = args.Data().As<External>();
	V8Instance *self = static_cast<V8Instance *>(data->Value());

	if (!args[0]->IsArray()) {
		std::string err_msg = "shuffle() requires an array as it's argument. You gave: " + std::string(*String::Utf8Value(args[0]->TypeOf(args.GetIsolate())));
		args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), err_msg.c_str(), NewStringType::kNormal).ToLocalChecked());
	}
	else {
		Local<Array> given_arr = Local<Array>::Cast(args[0]);
		const int length = given_arr->Length();
		Local<Array> return_arr = Array::New(args.GetIsolate(), length);

		std::vector<Local<Value>> cpp_arr;
		for (uint32_t i = 0; i < given_arr->Length(); i++) {
			cpp_arr.push_back(given_arr->Get(i));
		}

		std::shuffle(cpp_arr.begin(), cpp_arr.end(), self->rng);

		for (uint32_t i = 0; i < given_arr->Length(); i++) {
			return_arr->Set(i, cpp_arr[i]);
		}

		args.GetReturnValue().Set(return_arr);
	}
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
	Local<Context> context = Local<Context>::New(isolate, context_);
	Context::Scope context_scope(context);

	context->Global()->Set(
		String::NewFromUtf8(isolate, "input", NewStringType::kNormal).ToLocalChecked(),
		String::NewFromUtf8(isolate, args.c_str(), NewStringType::kNormal).ToLocalChecked()
	);
	Local<Object> user_obj = wrap_user(sender);
	context->Global()->Set(
		String::NewFromUtf8(isolate, "user", NewStringType::kNormal).ToLocalChecked(),
		user_obj
	);
	Local<Object> channel_obj = wrap_user(sender);
	context->Global()->Set(
		String::NewFromUtf8(isolate, "user", NewStringType::kNormal).ToLocalChecked(),
		user_obj
	);
	// TODO: 'message' object here too, although it's fairly pointless

	current_sender = sender;
	current_channel = channel;

	Logger::write("[v8] Preparing JS (guild " + (*guilds)[guild_id].id + ", channel " + channel->id + ")", Logger::LogLevel::Debug);

	Local<String> source = String::NewFromUtf8(isolate, js.c_str(), NewStringType::kNormal).ToLocalChecked();

	// compile
	Logger::write("[v8] Isolate nullptr? " + std::to_string(isolate == nullptr) + " Context empty? " + std::to_string(context.IsEmpty()), Logger::LogLevel::Debug);

	TryCatch compile_try_catch(isolate);
	Local<Script> script;

	auto begin = std::chrono::steady_clock::now();
	if (!Script::Compile(context, source).ToLocal(&script)) {
		String::Utf8Value error(compile_try_catch.Exception());

		std::string err_msg = *error;
		Logger::write("[v8] Compilation error: " + err_msg, Logger::LogLevel::Debug);
		DiscordAPI::send_message(channel->id, ":warning: **Compilation error:** `" + err_msg + "`");

		return;
	}

	TryCatch run_try_catch(isolate);
	MaybeLocal<Value> v = script->Run(context);
	if (v.IsEmpty()) {
		String::Utf8Value error(run_try_catch.Exception());

		std::string err_msg = *error;
		Logger::write("[v8] Runtime error: " + err_msg, Logger::LogLevel::Debug);
		DiscordAPI::send_message(channel->id, ":warning: **Runtime error:** `" + err_msg + "`");
	}

	auto end = std::chrono::steady_clock::now();
	long long time_taken = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
	Logger::write("[v8] Script compiled and run in " + std::to_string(time_taken) + "ms", Logger::LogLevel::Debug);

	current_sender = nullptr;
	current_channel = nullptr;

	if (print_text != "") {
		DiscordAPI::send_message(channel->id, print_text);
		print_text = "";
	}
}