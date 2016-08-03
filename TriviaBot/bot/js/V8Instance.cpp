#include <iostream>
#include <string>

#include "V8Instance.hpp"
#include "../APIHelper.hpp"

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
	std::cout << "Created isolate." << std::endl;

	Isolate::Scope isolate_scope(isolate);
	HandleScope handle_scope(isolate);

	// set global context
	Local<Context> context = create_context();
	context->Enter();
	Context::Scope context_scope(context);
	std::cout << "Created context and context scope." << std::endl;
}

v8::Local<v8::Context> V8Instance::create_context() {
	Local<ObjectTemplate> global = ObjectTemplate::New(isolate);
	// bind print() function
	Local<External> self = External::New(isolate, (void *) this);
	global->Set(String::NewFromUtf8(isolate, "print", NewStringType::kNormal).ToLocalChecked(), FunctionTemplate::New(isolate, V8Instance::js_print, self));
	std::cout << "Created global, assigned print function." << std::endl;

	return Context::New(isolate, NULL, global);
}

void V8Instance::clean_up() {
	std::cout << "Cleaning up." << std::endl;
	isolate->Exit();
	isolate->Dispose();
	delete array_buffer_allocator;
}

void V8Instance::reload() {
	clean_up();
	create();
}

void V8Instance::exec_js(std::string js, std::string channel_id) {
	std::cout << "Isolate nullptr? " << (isolate == nullptr) << std::endl;

	HandleScope handle_scope(isolate);
	Local<Context> context(isolate->GetCurrentContext());

	std::cout << "Executing js: " << js << std::endl;

	Local<String> source = String::NewFromUtf8(isolate, js.c_str(), NewStringType::kNormal).ToLocalChecked();
	std::cout << "String coverted" << std::endl;

	// compile
	std::cout << "Context empty? " << context.IsEmpty() << std::endl;

	TryCatch try_catch(isolate);
	Local<Script> script;

	if (!Script::Compile(context, source).ToLocal(&script)) {
		String::Utf8Value error(try_catch.Exception());
		std::cerr << "Error: " << *error << std::endl;

		return;
	}
	std::cout << "Compiled" << std::endl;

	// run
	script->Run(context);
	std::cout << "Ran" << std::endl;

	ah->send_message(channel_id, print_text);
	print_text = "";
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