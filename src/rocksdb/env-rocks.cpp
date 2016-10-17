#include <nan.h>

#include "env-rocks.h"

using namespace v8;
using namespace rocksdb;

namespace kv { namespace rocks {

void env::setup_export(v8::Handle<v8::Object>& exports) {
	// Prepare constructor template
	Local<FunctionTemplate> envTpl = Nan::New<FunctionTemplate>(env::ctor);
	envTpl->SetClassName(Nan::New("Env").ToLocalChecked());
	envTpl->InstanceTemplate()->SetInternalFieldCount(1);

	// Add functions to the prototype
	Nan::SetPrototypeMethod(envTpl, "open", env::open);

	// Set exports
	exports->Set(Nan::New("Env").ToLocalChecked(), envTpl->GetFunction());

}

NAN_METHOD(env::ctor) {
	Nan::HandleScope scope;

	env* ptr = new env();
	ptr->Wrap(info.This());
	info.GetReturnValue().Set(info.This());
}

NAN_METHOD(env::open) {
	Nan::HandleScope scope;

	env* envw = Nan::ObjectWrap::Unwrap<env>(info.This());

	Nan::Utf8String path(info[0]);
	DB* pdb;

	Options opt;
	opt.create_if_missing = true;
	opt.create_missing_column_families = true;
	if (info[1]->IsNumber()) opt.write_buffer_size = size_t(info[1]->NumberValue());

	Status s = DB::Open(opt, *path, envw->_desc, &envw->_handles, &pdb);
	if (!s.ok()) {
		Nan::ThrowError(s.ToString().c_str());
		return;
	}

	envw->_db = pdb;
	return;
}

env::env() : _db(NULL) {
	_desc.push_back(ColumnFamilyDescriptor(kDefaultColumnFamilyName, ColumnFamilyOptions()));
}

env::~env() {
	delete _db;
}

int env::register_db(const ColumnFamilyDescriptor& desc) {
	_desc.push_back(desc);
	return _desc.size() - 1;
}

} } // namespace kv | namespace rocks
