#include <nan.h>
#include <leveldb/comparator.h>
#include <leveldb/cache.h>

#include "../kv-types.h"

#include "batch-lvl.h"
#include "db-lvl.h"

using namespace v8;
using namespace kv;
using namespace kv::level;

#define DB_EXPORT(KT, VT) db<KT, VT>::setup_export(exports);
void kv::level::setup_db_export(v8::Handle<v8::Object>& exports) {
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

	Nan::Utf8String path(info[0]);
	db_type* pdb;

	option_type opt;
	if (info[1]->IsNumber()) opt.block_cache = leveldb::NewLRUCache(size_t(info[1]->NumberValue()));
	opt.create_if_missing = true;
	typedef lvl_rocks_comparator<K, comparator_type, slice_type> cmp;
	if (cmp::get_cmp()) opt.comparator = cmp::get_cmp();

	status_type s = db_type::Open(opt, *path, &pdb);

	if (!s.ok()) {
		Nan::ThrowError(s.ToString().c_str());
		return;
	}

	db* ptr = new db(pdb, opt.block_cache);
	ptr->Wrap(info.This());
	info.GetReturnValue().Set(info.This());
}

KVDB_METHOD(get) {
	Nan::HandleScope scope;

	db *dw = Nan::ObjectWrap::Unwrap<db>(info.This());

	K k(info[0]);
	slice_type key(k.data(), k.size());

	std::string val;
	status_type s = dw->_db->Get(readoption_type(), key, &val);

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
	slice_type key(k.data(), k.size()), val(v.data(), v.size());

	if (info[2]->IsObject()) {
		batch *bw = Nan::ObjectWrap::Unwrap<batch>(info[2]->ToObject());
		bw->_batch.Put(key, val);
	} else {
		status_type s = dw->_db->Put(writeoption_type(), key, val);

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
	slice_type key(k.data(), k.size());

	if (info[1]->IsObject()) {
		batch *bw = Nan::ObjectWrap::Unwrap<batch>(info[1]->ToObject());
		bw->_batch.Delete(key);
	} else {
		status_type s = dw->_db->Delete(writeoption_type(), key);

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

	status_type s = dw->_db->Write(writeoption_type(), &bw->_batch);

	if (!s.ok()) {
		Nan::ThrowError(s.ToString().c_str());
	}

	return;
}

template <class K, class V> db<K, V>::db(db_type* pdb, cache_type* pcache) : _db(pdb), _cache(pcache) {

}

template <class K, class V> db<K, V>::~db() {
	delete _db;
	delete _cache;
}
