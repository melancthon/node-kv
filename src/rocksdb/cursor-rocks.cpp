#include <nan.h>

#include "../kv-types.h"

#include "db-rocks.h"
#include "cursor-rocks.h"

using namespace v8;
using namespace rocksdb;

namespace kv { namespace rocks {

#define CURSOR_EXPORT(KT, VT) cursor<KT, VT>::setup_export(exports);
void setup_cursor_export(v8::Handle<v8::Object>& exports) {
	KV_TYPE_EACH(CURSOR_EXPORT);
}

template<class K, class V> void cursor<K, V>::setup_export(Handle<Object>& exports) {
	char class_name[64];
	sprintf(class_name, "Cursor_%s_%s", K::type_name, V::type_name);

	// Prepare constructor template
	Local<FunctionTemplate> cursorTpl = Nan::New<FunctionTemplate>(cursor::ctor);
	cursorTpl->SetClassName(Nan::New(class_name).ToLocalChecked());
	cursorTpl->InstanceTemplate()->SetInternalFieldCount(1);

	// Add functions to the prototype
	Nan::SetPrototypeMethod(cursorTpl, "key", cursor::key);
	Nan::SetPrototypeMethod(cursorTpl, "val", cursor::value);

	Nan::SetPrototypeMethod(cursorTpl, "next", cursor::next);
	Nan::SetPrototypeMethod(cursorTpl, "prev", cursor::prev);
	Nan::SetPrototypeMethod(cursorTpl, "first", cursor::first);
	Nan::SetPrototypeMethod(cursorTpl, "last", cursor::last);

	Nan::SetPrototypeMethod(cursorTpl, "gte", cursor::gte);

	// Set exports
	exports->Set(Nan::New(class_name).ToLocalChecked(), cursorTpl->GetFunction());
}

#define KVCURSOR cursor<K, V>
#define KVCURSOR_METHOD(fn) template <class K, class V> NAN_METHOD(KVCURSOR::fn)

KVCURSOR_METHOD(ctor) {
	Nan::HandleScope scope;

	db<K, V> *dw = Nan::ObjectWrap::Unwrap<db<K, V> >(info[0]->ToObject());
	iterator_type *cur = dw->_db->NewIterator(typename db<K, V>::readoption_type(), dw->_cf);

	cursor *ptr = new cursor(cur);
	ptr->Wrap(info.This());
	info.GetReturnValue().Set(info.This());
}

template<class K, class V> template<void (KVCURSOR::iterator_type::*FN)()> NAN_METHOD(KVCURSOR::cursorOp) {
	Nan::HandleScope scope;
	cursor *cw = Nan::ObjectWrap::Unwrap<cursor>(info.This());
	(cw->_cursor->*FN)();
	info.GetReturnValue().Set(Nan::New(cw->_cursor->Valid()));
}

KVCURSOR_METHOD(key) {
	Nan::HandleScope scope;

	cursor *cw = Nan::ObjectWrap::Unwrap<cursor>(info.This());
	if (!cw->_cursor->Valid()) {
		info.GetReturnValue().Set(Nan::Null());
	}

	typename db<K, V>::slice_type key(cw->_cursor->key());
	info.GetReturnValue().Set(K(key.data(), key.size()).v8value());
}

KVCURSOR_METHOD(value) {
	Nan::HandleScope scope;

	cursor *cw = Nan::ObjectWrap::Unwrap<cursor>(info.This());
	if (!cw->_cursor->Valid()) {
		info.GetReturnValue().Set(Nan::Null());
	}

	typename db<K, V>::slice_type val(cw->_cursor->value());
	info.GetReturnValue().Set(V(val.data(), val.size()).v8value());
}

KVCURSOR_METHOD(next) {
	return cursorOp<&iterator_type::Next>(info);
}

KVCURSOR_METHOD(prev) {
	return cursorOp<&iterator_type::Prev>(info);
}

KVCURSOR_METHOD(first) {
	return cursorOp<&iterator_type::SeekToFirst>(info);
}

KVCURSOR_METHOD(last) {
	return cursorOp<&iterator_type::SeekToLast>(info);
}

KVCURSOR_METHOD(gte) {
	Nan::HandleScope scope;

	cursor *cw = Nan::ObjectWrap::Unwrap<cursor>(info.This());

	K k(info[0]);
	typename db<K, V>::slice_type key(k.data(), k.size());
	cw->_cursor->Seek(key);

	info.GetReturnValue().Set(Nan::New(cw->_cursor->Valid()));
}

template <class K, class V> cursor<K, V>::cursor(iterator_type *cur) : _cursor(cur) {

}

template <class K, class V> cursor<K, V>::~cursor() {
	delete _cursor;
}

} } // namespace kv | namespace rocks
