#include <nan.h>
#include <lmdb/lmdb.h>

#include "env-lmdb.h"

using namespace v8;
using namespace kv;
using namespace kv::lmdb;

void env::setup_export(Handle<Object>& exports) {
	// Prepare constructor template
	Local<FunctionTemplate> envTpl = Nan::New<FunctionTemplate>(env::ctor);
	envTpl->SetClassName(Nan::New("Env").ToLocalChecked());
	envTpl->InstanceTemplate()->SetInternalFieldCount(1);

	// Add functions to the prototype
	Nan::SetPrototypeMethod(envTpl, "setMapSize", env::setMapSize);
	Nan::SetPrototypeMethod(envTpl, "setMaxDbs", env::setMaxDbs);

	Nan::SetPrototypeMethod(envTpl, "open", env::open);
	Nan::SetPrototypeMethod(envTpl, "close", env::close);
	Nan::SetPrototypeMethod(envTpl, "sync", env::sync);

	// Set exports
	exports->Set(Nan::New("Env").ToLocalChecked(), envTpl->GetFunction());
}

NAN_METHOD(env::ctor) {
	Nan::HandleScope();

	env *ptr = new env();
	int rc = mdb_env_create(&ptr->_env);

	if (rc != 0) {
		mdb_env_close(ptr->_env);
		Nan::ThrowError(mdb_strerror(rc));
		return; //return;
	}

	ptr->Wrap(info.This());
	info.GetReturnValue().Set(info.This());
}

NAN_METHOD(env::setMapSize) {
	Nan::HandleScope();

	env *ew = Nan::ObjectWrap::Unwrap<env>(info.This());

	if (!ew->_env) {
		Nan::ThrowError("The environment is already closed.");
		return; //return;
	}

	if (info[0]->IsNumber()) mdb_env_set_mapsize(ew->_env, size_t(info[0]->NumberValue()));
	return; //return;
}

NAN_METHOD(env::setMaxDbs) {
	Nan::HandleScope();

	env *ew = Nan::ObjectWrap::Unwrap<env>(info.This());

	if (!ew->_env) {
		Nan::ThrowError("The environment is already closed.");
		return;//return;
	}

	if (info[0]->IsNumber()) mdb_env_set_maxdbs(ew->_env, info[0]->Uint32Value());
	return; //return;
}

NAN_METHOD(env::open) {
	int rc = 0;
	Nan::HandleScope();

	env *ew = Nan::ObjectWrap::Unwrap<env>(info.This());
	if (!ew->_env) {
		Nan::ThrowError("The environment is already closed.");
		return;//return;
	}

	Nan::Utf8String path(info[0]);
	int flags = MDB_NOSYNC;
	rc = mdb_env_open(ew->_env, *path, flags, 0664);

	if (rc != 0) {
		mdb_env_close(ew->_env);
		ew->_env = NULL;
		Nan::ThrowError(mdb_strerror(rc));
		return;//return;
	}

	int cleared = 0;
	mdb_reader_check(ew->_env, &cleared);

	return;//return;
}

NAN_METHOD(env::close) {
	Nan::HandleScope();

	env *ew = Nan::ObjectWrap::Unwrap<env>(info.This());

	if (!ew->_env) {
		Nan::ThrowError("The environment is already closed.");
		return;//return;
	}

	if (ew->_read_lock) mdb_txn_abort(ew->_read_lock);
	ew->_read_lock = NULL;

	mdb_env_close(ew->_env);
	ew->_env = NULL;

	return;//return;
}

struct uv_env_sync {
	uv_work_t request;
	Nan::Callback* callback;
	env *ew;
	MDB_env *dbenv;
	int rc;
};

void sync_cb(uv_work_t *request) {
	// Performing the sync (this will be called on a separate thread)
	uv_env_sync *d = static_cast<uv_env_sync*>(request->data);
	d->rc = mdb_env_sync(d->dbenv, 1);
}

void after_sync_cb(uv_work_t *request, int) {
	// Executed after the sync is finished
	uv_env_sync *d = static_cast<uv_env_sync*>(request->data);
	const unsigned argc = 1;
	//Handle<Value> argv[argc];
	Local<Value> argv[argc];

	if (d->rc == 0) {
		argv[0] = Nan::Null();
	}
	else {
		argv[0] = Exception::Error(Nan::New<String>(mdb_strerror(d->rc)).ToLocalChecked());
	}

	d->callback->Call(argc, argv);
	delete d->callback;
	delete d;
}

NAN_METHOD(env::sync) {
	Nan::HandleScope();

	env *ew = Nan::ObjectWrap::Unwrap<env>(info.This());

	if (!ew->_env) {
		Nan::ThrowError("The environment is already closed.");
		return;//return;
	}

	Handle<Function> callback = Handle<Function>::Cast(info[0]);

	uv_env_sync *d = new uv_env_sync;
	d->request.data = d;
	d->ew = ew;
	d->dbenv = ew->_env;
	d->callback = new Nan::Callback(callback);

	uv_queue_work(uv_default_loop(), &d->request, sync_cb, after_sync_cb);

	return;//return;
}

env::env() : _env(NULL), _read_lock(NULL) {

}

env::~env() {
	if (_read_lock) mdb_txn_abort(_read_lock);
	if (_env) mdb_env_close(_env);
}

MDB_txn* env::require_readlock() {
	if (_read_lock) {
		mdb_txn_renew(_read_lock);
	} else {
		mdb_txn_begin(_env, NULL, MDB_RDONLY, &_read_lock);
	}

	return _read_lock;
}

void env::release_readlock() {
	mdb_txn_reset(_read_lock);
}
