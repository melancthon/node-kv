#include <nan.h>

#include "../kv-types.h"

#include "env-lmdb.h"
#include "txn-lmdb.h"
#include "db-lmdb.h"

using namespace v8;
using namespace kv;
using namespace kv::lmdb;

template<class T> int mdb_cmp_fn(const MDB_val *a, const MDB_val *b) {
	T ta((const char*)a->mv_data, a->mv_size), tb((const char*)b->mv_data, b->mv_size);
	return ta.compare(tb);
}

template<class T> struct mdb_cmp_setter {
	static void set_cmp(MDB_txn*, MDB_dbi) { }
	static void set_dup_cmp(MDB_txn*, MDB_dbi) { }
};

template<class N> struct mdb_cmp_setter<number_type<N> > {
	static void set_cmp(MDB_txn* txn, MDB_dbi dbi) {
		mdb_set_compare(txn, dbi, mdb_cmp_fn<number_type<N> >);
	}

	static void set_dup_cmp(MDB_txn* txn, MDB_dbi dbi) {
		mdb_set_dupsort(txn, dbi, mdb_cmp_fn<number_type<N> >);
	}
};

#define DB_EXPORT(KT, VT) db<KT, VT>::setup_export(exports);
void kv::lmdb::setup_db_export(v8::Handle<v8::Object>& exports) {
	KV_TYPE_EACH(DB_EXPORT);
}

template <class K, class V> void db<K, V>::setup_export(Handle<Object>& exports) {
	char class_name[64];
	sprintf(class_name, "DB_%s_%s", K::type_name, V::type_name);

	// Prepare constructor template
	Local<FunctionTemplate> dbiTpl = Nan::New<FunctionTemplate>(db::ctor);//.ToLocalChecked();
	dbiTpl->SetClassName(Nan::New(class_name).ToLocalChecked());
	dbiTpl->InstanceTemplate()->SetInternalFieldCount(1);

	// Add functions to the prototype
	Nan::SetPrototypeMethod(dbiTpl, "close", db::close);
	Nan::SetPrototypeMethod(dbiTpl, "get", db::get);
	Nan::SetPrototypeMethod(dbiTpl, "put", db::put);
	Nan::SetPrototypeMethod(dbiTpl, "del", db::del);
	Nan::SetPrototypeMethod(dbiTpl, "exists", db::exists);
	// TODO: wrap mdb_stat too

	// Set exports
	exports->Set(Nan::New(class_name).ToLocalChecked(), dbiTpl->GetFunction());
}

#define KVDB db<K, V>
#define KVDB_METHOD(fn) template <class K, class V> NAN_METHOD(KVDB::fn)

KVDB_METHOD(ctor) {
	int rc = 0;
	Nan::HandleScope scope;

	MDB_txn *txn;
	MDB_dbi dbi;
	env *ew = Nan::ObjectWrap::Unwrap<env>(info[0]->ToObject());

	if (info[1]->IsObject()) {
		Local<Object> options = info[1]->ToObject();
		Nan::Utf8String name(options->Get(Nan::New("name").ToLocalChecked()));
		bool allowdup = options->Get(Nan::New("allowDup").ToLocalChecked())->BooleanValue();

		// Open transaction
		rc = mdb_txn_begin(ew->_env, NULL, 0, &txn);
		if (rc != 0) {
			mdb_txn_abort(txn);
			Nan::ThrowError(mdb_strerror(rc));
			return; //return;
		}

		// Open database
		rc = mdb_dbi_open(txn, *name, allowdup ? MDB_CREATE | MDB_DUPSORT : MDB_CREATE, &dbi);
		if (rc != 0) {
			mdb_txn_abort(txn);
			Nan::ThrowError(mdb_strerror(rc));
			return;//return;
		}

		// Set compare function.
		mdb_cmp_setter<K>::set_cmp(txn, dbi);
		if (allowdup) mdb_cmp_setter<V>::set_dup_cmp(txn, dbi);

		// Commit transaction
		rc = mdb_txn_commit(txn);
		if (rc != 0) {
			Nan::ThrowError(mdb_strerror(rc));
			return;//return;
		}
	}
	else {
		Nan::ThrowError("Invalid parameters.");
		return; //return;
	}

	db* ptr = new db(ew, dbi);
	ptr->Wrap(info.This());
	info.GetReturnValue().Set(info.This());
}

KVDB_METHOD(close) {
	Nan::HandleScope scope;

	db *dw = Nan::ObjectWrap::Unwrap<db>(info.This());
	mdb_dbi_close(dw->_env->_env, dw->_dbi);

	if (dw->_cur) {
		mdb_cursor_close(dw->_cur);
		dw->_cur = NULL;
	}

	return; //return;
}

class kv::lmdb::txn_scope {
public:
	txn_scope(Local<Value> arg, MDB_env *env) : _env(NULL), _txn(NULL), _readonly(false), _created(false), _commit(false) {
		if (arg->IsObject()) {
			_txn = Nan::ObjectWrap::Unwrap<txn>(arg->ToObject())->_txn;
		} else {
			_created = true;
			mdb_txn_begin(env, NULL, 0, &_txn);
		}
	}

	txn_scope(Local<Value> arg, env *env) : _env(env), _txn(NULL), _readonly(true), _created(false), _commit(false) {
		if (arg->IsObject()) {
			_txn = Nan::ObjectWrap::Unwrap<txn>(arg->ToObject())->_txn;
		}
		else {
			_created = true;
			_txn = env->require_readlock();
		}
	}

	~txn_scope() {
		if (_created) {
			if (_readonly) {
				_env->release_readlock();
			} else {
				if (!_commit) mdb_txn_abort(_txn);
				else mdb_txn_commit(_txn);
			}
		}
	}

	bool is_readonly() {
		return _readonly;
	}

	bool is_created() {
		return _created;
	}

	MDB_txn *operator*() {
		return _txn;
	}

	void commit() {
		_commit = true;
	}

private:
	env* _env;
	MDB_txn *_txn;
	bool _readonly;
	bool _created;
	bool _commit;
};

KVDB_METHOD(get) {
	Nan::HandleScope scope;

	db *dw = Nan::ObjectWrap::Unwrap<db>(info.This());
	K key = K(info[0]);
	txn_scope tc(info[1], dw->_env);

	MDB_val k, v;
	k.mv_data = (void*)key.data();
	k.mv_size = key.size();

	int rc = mdb_get(*tc, dw->_dbi, &k, &v);

	if (rc == MDB_NOTFOUND) {
		return info.GetReturnValue().Set(Nan::Null());
	}

	if (rc != 0) {
		Nan::ThrowError(mdb_strerror(rc));
		return;
	}

	V val((const char*)v.mv_data, v.mv_size);
	info.GetReturnValue().Set(val.v8value());
}

KVDB_METHOD(put) {
	Nan::HandleScope scope;

	db *dw = Nan::ObjectWrap::Unwrap<db>(info.This());
	K key = K(info[0]);
	V val = V(info[1]);
	txn_scope tc(info[2], dw->_env->_env);

	MDB_val k, v;
	k.mv_data = (void*)key.data();
	k.mv_size = key.size();
	v.mv_data = (void*)val.data();
	v.mv_size = val.size();

	int rc = mdb_put(*tc, dw->_dbi, &k, &v, 0);
	if (rc != 0) {
		Nan::ThrowError(mdb_strerror(rc));
		return;
	}

	tc.commit();
	info.GetReturnValue().Set(Nan::New(true));
}

KVDB_METHOD(del) {
	Nan::HandleScope scope;

	db *dw = Nan::ObjectWrap::Unwrap<db>(info.This());
	K key = K(info[0]);
	V val = V(info[1]);
	txn_scope tc(info[2], dw->_env->_env);

	MDB_val k, v;
	k.mv_data = (void*)key.data();
	k.mv_size = key.size();
	v.mv_data = (void*)val.data(); 
	v.mv_size = val.size();

	int rc = mdb_del(*tc, dw->_dbi, &k, &v)

	if (rc == MDB_NOTFOUND) {
		return info.GetReturnValue().Set(Nan::New(false));
	}

	if (rc != 0) {
		Nan::ThrowError(mdb_strerror(rc));
		return;
	}

	tc.commit();
	info.GetReturnValue().Set(Nan::New(true));
}

KVDB_METHOD(exists) {
	Nan::HandleScope scope;

	db *dw = Nan::ObjectWrap::Unwrap<db>(info.This());
	K key = K(info[0]);
	V val = V(info[1]);

	MDB_val k, v;
	k.mv_data = (void*)key.data();
	k.mv_size = key.size();
	v.mv_data = (void*)val.data();
	v.mv_size = val.size();

	txn_scope tc(info[2], dw->_env);
	MDB_cursor *cur = NULL;
	if (tc.is_created()) {
		if (!dw->_cur) mdb_cursor_open(*tc, dw->_dbi, &dw->_cur);
		else mdb_cursor_renew(*tc, dw->_cur);
		cur = dw->_cur;
	} else {
		mdb_cursor_open(*tc, dw->_dbi, &cur);
	}

	int rc = mdb_cursor_get(cur, &k, &v, MDB_GET_BOTH);
	if (!tc.is_created()) {
		mdb_cursor_close(cur);
	}

	if (rc == MDB_NOTFOUND) {
		return info.GetReturnValue().Set(Nan::New(false));
	} else if (rc == 0) {
		return info.GetReturnValue().Set(Nan::New(true));
	} else {
		Nan::ThrowError(mdb_strerror(rc));
		return;
	}
}

template <class K, class V> db<K, V>::db(env* env, MDB_dbi dbi) : _dbi(dbi), _env(env), _cur(NULL) {

}

template <class K, class V> db<K, V>::~db() {
	if (_cur) mdb_cursor_close(_cur);
}
