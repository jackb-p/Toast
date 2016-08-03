#ifndef BOT_JS_V8INSTANCE
#define BOT_JS_V8INSTANCE

#include <memory>

#include <include/v8.h>
#include <include/libplatform/libplatform.h>

class APIHelper;

class V8Instance {
public:
	V8Instance(std::shared_ptr<APIHelper> ah);
	~V8Instance();
	void reload();
	void exec_js(std::string js, std::string channel_id);

private:
	void clean_up();
	void create();
	v8::Local<v8::Context> create_context();
	static void js_print(const v8::FunctionCallbackInfo<v8::Value> &args);

	v8::ArrayBuffer::Allocator *array_buffer_allocator;
	v8::Isolate *isolate;

	std::shared_ptr<APIHelper> ah;
	std::string print_text;
};

#endif