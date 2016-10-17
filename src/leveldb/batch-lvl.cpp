#include <nan.h>

#include "../kv-types.h"

#include "batch-lvl.h"

using namespace v8;
using namespace kv;
using namespace level;

void batch::setup_export(Handle<Object>& exports) {
	// Prepare constructor template
	Local<FunctionTemplate> batchTpl = Nan::New<FunctionTemplate>(batch::ctor);
	batchTpl->SetClassName(Nan::New("Batch").ToLocalChecked());
	batchTpl->InstanceTemplate()->SetInternalFieldCount(1);

	// Add functions to the prototype
	Nan::SetPrototypeMethod(batchTpl, "clear", batch::clear);

	// Set exports
	exports->Set(Nan::New("Batch").ToLocalChecked(), batchTpl->GetFunction());
}

NAN_METHOD(batch::ctor) {
	Nan::HandleScope scope;
	batch* ptr = new batch;
	ptr->Wrap(info.This());
	info.GetReturnValue().Set(info.This());
}

NAN_METHOD(batch::clear) {
	Nan::HandleScope scope;

	batch* bat = Nan::ObjectWrap::Unwrap<batch>(info.This());
	bat->_batch.Clear();

	return;
}
