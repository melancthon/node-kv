#include <nan.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>
#include <rocksdb/comparator.h>

#include "../kv-types.h"

#include "env-rocks.h"
#include "batch-rocks.h"
#include "db-rocks.h"

using namespace v8;
using namespace rocksdb;

namespace kv { namespace rocks {

#define DB_EXPORT(KT, VT) db<KT, VT>::setup_export(exports);
void setup_db_export(v8::Handle<v8::Object>& exports) {
	KV_TYPE_EACH(DB_EXPORT);
}

template <class K, class V> void db<K, V>::setup_export(Handle<Object>& exports) {
	char class_name[64];
	sprintf(class_name, "DB_%s_%s", K::type_name, V::type_name);

	// Prepare constructor template
	Local<FunctionTemplate> dbiTpl = Nan::New<FunctionTemplate>(db::ctor);
	dbiTpl->SetClassName(Nan::New(class_name).ToLocalChecked());
	dbiTpl->InstanceTemplate()->SetInternalFieldCount(1);

	// Add functions to the prototype
	Nan::SetPrototypeMethod(dbiTpl, "init", db::init);

	Nan::SetPrototypeMethod(dbiTpl, "get", db::get);
	Nan::SetPrototypeMethod(dbiTpl, "put", db::put);
	Nan::SetPrototypeMethod(dbiTpl, "del", db::del);
	Nan::SetPrototypeMethod(dbiTpl, "write", db::write);

	// Set exports
	exports->Set(Nan::New(class_name).ToLocalChecked(), dbiTpl->GetFunction());
}

#define KVDB db<K, V>
#define KVDB_METHOD(fn) template <class K, class V> NAN_METHOD(KVDB::fn)

KVDB_METHOD(ctor) {
	Nan::HandleScope scope;

	Nan::Utf8String name(info[1]);
	env *ew = Nan::ObjectWrap::Unwrap<env>(info[0]->ToObject());

	ColumnFamilyOptions opt;
	typedef lvl_rocks_comparator<K, Comparator, Slice> cmp;
	if (cmp::get_cmp()) opt.comparator = cmp::get_cmp();

	int idx = ew->register_db(ColumnFamilyDescriptor(*name, opt));

	db* ptr = new db(idx);
	ptr->Wrap(info.This());
	info.GetReturnValue().Set(info.This());
}

KVDB_METHOD(init) {
	Nan::HandleScope scope;

	db *dw = Nan::ObjectWrap::Unwrap<db>(info.This());
	env *ew = Nan::ObjectWrap::Unwrap<env>(info[0]->ToObject());
	dw->_db = ew->_db;
	dw->_cf = ew->_handles[dw->_cfidx];

	return;
}

KVDB_METHOD(get) {
	Nan::HandleScope scope;

	db *dw = Nan::ObjectWrap::Unwrap<db>(info.This());

	K k(info[0]);
	Slice key(k.data(), k.size());

	std::string val;
	Status s = dw->_db->Get(ReadOptions(), dw->_cf, key, &val);

	if (s.IsNotFound()) {
		info.GetReturnValue().Set(Nan::Null());
	}

	if (!s.ok()) {
		Nan::ThrowError(s.ToString().c_str());
		return;
	}

	V v(val.data(), val.size());
	info.GetReturnValue().Set(v.v8value());
}

KVDB_METHOD(put) {
	Nan::HandleScope scope;

	db *dw = Nan::ObjectWrap::Unwrap<db>(info.This());

	K k(info[0]);
	V v(info[1]);
	Slice key(k.data(), k.size()), val(v.data(), v.size());

	if (info[2]->IsObject()) {
		batch *bw = Nan::ObjectWrap::Unwrap<batch>(info[2]->ToObject());
		bw->_batch.Put(dw->_cf, key, val);
	} else {
		Status s = dw->_db->Put(WriteOptions(), dw->_cf, key, val);

		if (!s.ok()) {
			Nan::ThrowError(s.ToString().c_str());
		}
	}

	return;
}

KVDB_METHOD(del) {
	Nan::HandleScope scope;

	db *dw = Nan::ObjectWrap::Unwrap<db>(info.This());

	K k(info[0]);
	Slice key(k.data(), k.size());

	if (info[1]->IsObject()) {
		batch *bw = Nan::ObjectWrap::Unwrap<batch>(info[1]->ToObject());
		bw->_batch.Delete(dw->_cf, key);
	} else {
		Status s = dw->_db->Delete(WriteOptions(), dw->_cf, key);

		if (!s.ok()) {
			Nan::ThrowError(s.ToString().c_str());
		}
	}

	return;
}

KVDB_METHOD(write) {
	Nan::HandleScope scope;

	db *dw = Nan::ObjectWrap::Unwrap<db>(info.This());
	batch *bw = Nan::ObjectWrap::Unwrap<batch>(info[0]->ToObject());

	Status s = dw->_db->Write(WriteOptions(), &bw->_batch);

	if (!s.ok()) {
		Nan::ThrowError(s.ToString().c_str());
	}

	return;
}

template <class K, class V> db<K, V>::db(int idx) : _cfidx(idx), _db(NULL), _cf(NULL) {

}

template <class K, class V> db<K, V>::~db() {
	delete _cf;
}

} } // namespace kv | namespace rocks
