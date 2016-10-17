#include <nan.h>

#include "env-lmdb.h"
#include "txn-lmdb.h"

using namespace v8;
using namespace kv;
using namespace kv::lmdb;

void txn::setup_export(Handle<Object>& exports) {
	// Prepare constructor template
	Local<FunctionTemplate> txnTpl = Nan::New<FunctionTemplate>(txn::ctor);//.ToLocalChecked();
	txnTpl->SetClassName(Nan::New("Txn").ToLocalChecked());
	txnTpl->InstanceTemplate()->SetInternalFieldCount(1);

	// Add functions to the prototype
	Nan::SetPrototypeMethod(txnTpl, "commit", txn::commit);
	Nan::SetPrototypeMethod(txnTpl, "abort", txn::abort);
	Nan::SetPrototypeMethod(txnTpl, "reset", txn::reset);
	Nan::SetPrototypeMethod(txnTpl, "renew", txn::renew);

	// Set exports
	exports->Set(Nan::New("Txn").ToLocalChecked(), txnTpl->GetFunction());
}

NAN_METHOD(txn::ctor) {
	Nan::HandleScope();

	env *ew = Nan::ObjectWrap::Unwrap<env>(info[0]->ToObject());
	txn *ptr = new txn(ew->_env, info[1]->BooleanValue());

	int rc = mdb_txn_begin(ptr->_env, NULL, ptr->_readonly ? MDB_RDONLY : 0, &ptr->_txn);

	if (rc != 0) {
		Nan::ThrowError(mdb_strerror(rc));
		return;//return;
	}

	ptr->Wrap(info.This());
	info.GetReturnValue().Set(info.This());
}

NAN_METHOD(txn::commit) {
	Nan::HandleScope();

	txn *tw = Nan::ObjectWrap::Unwrap<txn>(info.This());

	if (!tw->_txn) {
		Nan::ThrowError("The transaction is already closed.");
		return;//return;
	}

	int rc = mdb_txn_commit(tw->_txn);
	tw->_txn = NULL;
	if (rc != 0) {
		Nan::ThrowError(mdb_strerror(rc));
		return;//return;
	}

	return;//return;
}

NAN_METHOD(txn::abort) {
	Nan::HandleScope();

	txn *tw = Nan::ObjectWrap::Unwrap<txn>(info.This());

	if (!tw->_txn) {
		Nan::ThrowError("The transaction is already closed.");
		return;//return;
	}

	mdb_txn_abort(tw->_txn);
	tw->_txn = NULL;

	return;//return;
}

NAN_METHOD(txn::reset) {
	Nan::HandleScope();

	txn *tw = Nan::ObjectWrap::Unwrap<txn>(info.This());

	if (!tw->_txn) {
		Nan::ThrowError("The transaction is already closed.");
		return;//return;
	}

	if (!tw->_readonly) {
		Nan::ThrowError("Only readonly transaction can be reset.");
		return;//return;
	}

	mdb_txn_reset(tw->_txn);
	return;//return;
}

NAN_METHOD(txn::renew) {
	Nan::HandleScope();

	txn *tw = Nan::ObjectWrap::Unwrap<txn>(info.This());

	if (tw->_readonly) {
		if (!tw->_txn) {
			Nan::ThrowError("The transaction is already closed.");
			return;//return;
		}

		int rc = mdb_txn_renew(tw->_txn);
		if (rc != 0) {
			Nan::ThrowError(mdb_strerror(rc));
			return;//return;
		}
	}
	else {
		if (tw->_txn) {
			Nan::ThrowError("The transaction is still opened.");
			return;//return;
		}

		int rc = mdb_txn_begin(tw->_env, NULL, 0, &tw->_txn);
		if (rc != 0) {
			Nan::ThrowError(mdb_strerror(rc));
			return;//return;
		}
	}

	return;//return;
}

txn::txn(MDB_env* env, bool readonly) : _readonly(readonly), _txn(NULL), _env(env) {

}

txn::~txn() {
	if (_txn) {
		printf("LMDB_txn warning: unclosed txn aborted by dtor.");
		mdb_txn_abort(_txn);
	}
}
