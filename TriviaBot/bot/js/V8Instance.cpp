#include <iostream>
#include <string>

#include "V8Instance.hpp"
#include "../APIHelper.hpp"
#include "../Logger.hpp"

using namespace v8;

V8Instance::V8Instance(std::shared_ptr<APIHelper> ah) {
	this->ah = ah;
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
	global->Set(String::NewFromUtf8(isolate, "print", NewStringType::kNormal).ToLocalChecked(), FunctionTemplate::New(isolate, V8Instance::js_print, self));
	Logger::write("[v8] Created global obj, linked print function", Logger::LogLevel::Debug);

	return Context::New(isolate, NULL, global);
}

void V8Instance::clean_up() {
	Logger::write("[v8] Cleaning up", Logger::LogLevel::Debug);
	isolate->Exit();
	isolate->Dispose();
	delete array_buffer_allocator;
}

void V8Instance::reload() {
	clean_up();
	create();
}

void V8Instance::exec_js(std::string js, std::string channel_id) {
	HandleScope handle_scope(isolate);
	Local<Context> context(isolate->GetCurrentContext());

	Logger::write("[v8] Executing JS: " + js, Logger::LogLevel::Debug);

	Local<String> source = String::NewFromUtf8(isolate, js.c_str(), NewStringType::kNormal).ToLocalChecked();

	// compile
	Logger::write("[v8] Isolate nullptr? " + std::to_string(isolate == nullptr) + " Context empty? " + std::to_string(context.IsEmpty()), Logger::LogLevel::Debug);

	TryCatch compile_try_catch(isolate);
	Local<Script> script;

	if (!Script::Compile(context, source).ToLocal(&script)) {
		String::Utf8Value error(compile_try_catch.Exception());

		std::string err_msg = *error;
		Logger::write("[v8] Compilation error: " + err_msg, Logger::LogLevel::Debug);
		ah->send_message(channel_id, ":warning: **Compilation error:** `" + err_msg + "`");

		return;
	}

	TryCatch run_try_catch(isolate);
	MaybeLocal<Value> v = script->Run(context);
	if (v.IsEmpty()) {
		String::Utf8Value error(run_try_catch.Exception());

		std::string err_msg = *error;
		Logger::write("[v8] Runtime error: " + err_msg, Logger::LogLevel::Debug);
		ah->send_message(channel_id, ":warning: **Runtime error:** `" + err_msg + "`");

		return;
	}

	Logger::write("[v8] Script compiled and run", Logger::LogLevel::Debug);

	if (print_text != "") {
		ah->send_message(channel_id, print_text);
		print_text = "";
	}
}

void V8Instance::js_print(const v8::FunctionCallbackInfo<v8::Value> &args) {
	auto data = args.Data().As<v8::External>();
	V8Instance *self = static_cast<V8Instance *>(data->Value());

	std::string output = "";
	for (int i = 0; i < args.Length(); i++) {
		v8::HandleScope handle_scope(args.GetIsolate());
		v8::String::Utf8Value str(args[i]);
		self->print_text += *str;
	}
}