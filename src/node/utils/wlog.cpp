#include "../stdafx.h"

#include "wlog.h"

void WLogger::Init(v8::Handle<v8::Object> exports){
	METHOD_BEGIN();

	v8::Local<v8::String> className = Nan::New("Logger").ToLocalChecked();

	// Basic instance setup
	v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);

	tpl->SetClassName(className);
	tpl->InstanceTemplate()->SetInternalFieldCount(1); // req'd by ObjectWrap

	// Prototype method bindings
	Nan::SetPrototypeMethod(tpl, "start", Start);
	Nan::SetPrototypeMethod(tpl, "stop", Stop);
	Nan::SetPrototypeMethod(tpl, "clear", Clear);

	// Store the constructor in the target bindings.
	constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

	exports->Set(className, tpl->GetFunction());
}

NAN_METHOD(WLogger::New) {
	METHOD_BEGIN();

	try{
		WLogger *obj = new WLogger();
		obj->data_ = new Logger();

		obj->Wrap(info.This());

		info.GetReturnValue().Set(info.This());
		return;
	}
	TRY_END();	
}

NAN_METHOD(WLogger::Start) {
	METHOD_BEGIN();

	try {
		UNWRAP_DATA(Logger);

		LOGGER_ARG("filename");
#if defined(OPENSSL_SYS_WINDOWS)
		wchar_t * wCont = (wchar_t *)* v8::String::Value(info[0]->ToString());
		char filename[MAX_PATH];
		std::wcstombs(filename, wCont, MAX_PATH);
#else
		v8::String::Utf8Value v8Filename(info[0]->ToString());
		char *filename = *v8Filename;
#endif // OPENSSL_SYS_WINDOWS
		v8::String::Utf8Value v8Filename(info[0]->ToString());

		LOGGER_ARG("level");
		int level = info[1]->ToNumber()->Int32Value();

		_this->start((const char *)filename, level);

		info.GetReturnValue().Set(info.This());
	}
	TRY_END();
}

NAN_METHOD(WLogger::Stop)
{
	try {
		UNWRAP_DATA(Logger);

		_this->stop();

		info.GetReturnValue().Set(info.This());
	}
	TRY_END();	
}

NAN_METHOD(WLogger::Clear)
{
	try {
		UNWRAP_DATA(Logger);

		_this->clear();

		info.GetReturnValue().Set(info.This());
	}
	TRY_END();
}
