#include <nan.h>

#include "../kv-types.h"

#include "txn-lmdb.h"
#include "db-lmdb.h"
#include "cursor-lmdb.h"

using namespace v8;
using namespace kv;
using namespace kv::lmdb;

#define CURSOR_EXPORT(KT, VT) cursor<KT, VT>::setup_export(exports);
void kv::lmdb::setup_cursor_export(v8::Handle<v8::Object>& exports) {
	KV_TYPE_EACH(CURSOR_EXPORT);
}

template<class K, class V> void cursor<K, V>::setup_export(Handle<Object>& exports) {
	char class_name[64];
	sprintf(class_name, "Cursor_%s_%s", K::type_name, V::type_name);

	// Prepare constructor template
	Local<FunctionTemplate> cursorTpl = Nan::New<FunctionTemplate>(cursor::ctor);//.ToLocalChecked();
	cursorTpl->SetClassName(Nan::New(class_name).ToLocalChecked());
	cursorTpl->InstanceTemplate()->SetInternalFieldCount(1);

	// Add functions to the prototype
	Nan::SetPrototypeMethod(cursorTpl, "close", cursor::close);

	Nan::SetPrototypeMethod(cursorTpl, "del", cursor::del);
	Nan::SetPrototypeMethod(cursorTpl, "key", cursor::key);
	Nan::SetPrototypeMethod(cursorTpl, "val", cursor::value);

	Nan::SetPrototypeMethod(cursorTpl, "next", cursor::next);
	Nan::SetPrototypeMethod(cursorTpl, "prev", cursor::prev);
	Nan::SetPrototypeMethod(cursorTpl, "nextDup", cursor::nextDup);
	Nan::SetPrototypeMethod(cursorTpl, "prevDup", cursor::prevDup);

	Nan::SetPrototypeMethod(cursorTpl, "seek", cursor::seek);
	Nan::SetPrototypeMethod(cursorTpl, "gte", cursor::gte);
	Nan::SetPrototypeMethod(cursorTpl, "first", cursor::first);
	Nan::SetPrototypeMethod(cursorTpl, "last", cursor::last);

	// Set exports
	exports->Set(Nan::New(class_name).ToLocalChecked(), cursorTpl->GetFunction());
}

#define KVCURSOR cursor<K, V>
#define KVCURSOR_METHOD(fn) template <class K, class V> NAN_METHOD(KVCURSOR::fn)

KVCURSOR_METHOD(ctor) {
	Nan::HandleScope();

	db<K, V> *dw = Nan::ObjectWrap::Unwrap<db<K, V> >(info[0]->ToObject());
	txn *tw = Nan::ObjectWrap::Unwrap<txn>(info[1]->ToObject());

	MDB_cursor *cur = NULL;
	int rc = mdb_cursor_open(tw->_txn, dw->_dbi, &cur);
	if (rc != 0) {
		Nan::ThrowError(mdb_strerror(rc));
		return; //return;
	}

	cursor *ptr = new cursor(cur);
	ptr->Wrap(info.This());
	info.GetReturnValue().Set(info.This());
}

KVCURSOR_METHOD(close) {
	Nan::HandleScope();

	cursor *cw = Nan::ObjectWrap::Unwrap<cursor>(info.This());
	mdb_cursor_close(cw->_cursor);
	cw->_cursor = NULL;

	return; //return;
}

KVCURSOR_METHOD(del) {
	Nan::HandleScope();

	cursor *cw = Nan::ObjectWrap::Unwrap<cursor>(info.This());
	int rc = mdb_cursor_del(cw->_cursor, 0);
	if (rc != 0) {
		Nan::ThrowError(mdb_strerror(rc));
		return;//return;
	}

	return;//return;
}

template<class K, class V> template<MDB_cursor_op OP> NAN_METHOD(KVCURSOR::cursorOp) {
	Nan::HandleScope();

	cursor *cw = Nan::ObjectWrap::Unwrap<cursor>(info.This());

	MDB_val key = { 0, 0 }, data = { 0, 0 };
	int rc = mdb_cursor_get(cw->_cursor, &key, &data, OP);
	if (rc != 0 && rc != MDB_NOTFOUND) {
		Nan::ThrowError(mdb_strerror(rc));
		return;//return;
	}

	info.GetReturnValue().Set(Nan::New(rc != MDB_NOTFOUND));//.ToLocalChecked());
}

template<class K, class V> template<MDB_cursor_op OP> NAN_METHOD(KVCURSOR::cursorKeyOp) {
	Nan::HandleScope();

	cursor *cw = Nan::ObjectWrap::Unwrap<cursor>(info.This());

	K k = K(info[0]);

	MDB_val key = { 0, 0 }, data = { 0, 0 };
	key.mv_data = (void*)k.data();
	key.mv_size = k.size();

	int rc = mdb_cursor_get(cw->_cursor, &key, &data, OP);
	if (rc != 0 && rc != MDB_NOTFOUND) {
		Nan::ThrowError(mdb_strerror(rc));
		return;//return;
	}

	info.GetReturnValue().Set(Nan::New(rc != MDB_NOTFOUND));//.ToLocalChecked());
}

template<class K, class V> template<MDB_cursor_op OP> NAN_METHOD(KVCURSOR::cursorKeyValOp) {
	Nan::HandleScope();

	cursor *cw = Nan::ObjectWrap::Unwrap<cursor>(info.This());

	K k = K(info[0]);
	V v = V(info[1]);

	MDB_val key = { 0, 0 }, data = { 0, 0 };
	key.mv_data = (void*)k.data();
	key.mv_size = k.size();
	data.mv_data = (void*)v.data();
	data.mv_size = v.size();

	int rc = mdb_cursor_get(cw->_cursor, &key, &data, OP);
	if (rc != 0 && rc != MDB_NOTFOUND) {
		Nan::ThrowError(mdb_strerror(rc));
		return;//return;
	}

	info.GetReturnValue().Set(Nan::New(rc != MDB_NOTFOUND));//.ToLocalChecked());
}

KVCURSOR_METHOD(key) {
	Nan::HandleScope();

	cursor *cw = Nan::ObjectWrap::Unwrap<cursor>(info.This());

	MDB_val key = { 0, 0 }, data = { 0, 0 };
	int rc = mdb_cursor_get(cw->_cursor, &key, &data, MDB_GET_CURRENT);

	if (rc == MDB_NOTFOUND) {
		return info.GetReturnValue().Set(Nan::Null());
	}

	if (rc != 0) {
		Nan::ThrowError(mdb_strerror(rc));
		return;//return;
	}

	info.GetReturnValue().Set(K((const char*)key.mv_data, key.mv_size).v8value());
}

KVCURSOR_METHOD(value) {
	Nan::HandleScope();

	cursor *cw = Nan::ObjectWrap::Unwrap<cursor>(info.This());

	MDB_val key = { 0, 0 }, data = { 0, 0 };
	int rc = mdb_cursor_get(cw->_cursor, &key, &data, MDB_GET_CURRENT);

	if (rc == MDB_NOTFOUND) {
		return info.GetReturnValue().Set(Nan::Null());
	}

	if (rc != 0) {
		Nan::ThrowError(mdb_strerror(rc));
		return;//return;
	}

	info.GetReturnValue().Set(V((const char*)data.mv_data, data.mv_size).v8value());
}

KVCURSOR_METHOD(next) { return cursorOp<MDB_NEXT>(info); }
KVCURSOR_METHOD(prev) { return cursorOp<MDB_PREV>(info); }
KVCURSOR_METHOD(nextDup) { return cursorOp<MDB_NEXT_DUP>(info); }
KVCURSOR_METHOD(prevDup) { return cursorOp<MDB_PREV_DUP>(info); }

KVCURSOR_METHOD(first) { return cursorOp<MDB_FIRST>(info); }
KVCURSOR_METHOD(last) { return cursorOp<MDB_LAST>(info); }
KVCURSOR_METHOD(firstDup) { return cursorOp<MDB_FIRST_DUP>(info); }
KVCURSOR_METHOD(lastDup) { return cursorOp<MDB_LAST_DUP>(info); }

KVCURSOR_METHOD(seek) {
	return info.Length() == 1 ? cursorKeyOp<MDB_SET>(info) : cursorKeyValOp<MDB_GET_BOTH>(info);
}

KVCURSOR_METHOD(gte) {
	return info.Length() == 1 ? cursorKeyOp<MDB_SET_RANGE>(info) : cursorKeyValOp<MDB_GET_BOTH_RANGE>(info);
}

template <class K, class V> cursor<K, V>::cursor(MDB_cursor *cur) : _cursor(cur) {

}

template <class K, class V> cursor<K, V>::~cursor() {
	if (_cursor) {
		printf("LMDB_cursor warning: unclosed cursor closed by dtor.\n");
		mdb_cursor_close(_cursor);
	}
}
