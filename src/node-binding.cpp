#include <nan.h>

#include "lmdb/setup-lmdb.h"
#include "leveldb/setup-lvl.h"
#include "rocksdb/setup-rocks.h"

using namespace v8;

void init(v8::Handle<v8::Object> exports) {
	exports->Set(Nan::New("lmdb").ToLocalChecked(), kv::lmdb::v8export());
	exports->Set(Nan::New("leveldb").ToLocalChecked(), kv::level::v8export());
	exports->Set(Nan::New("rocksdb").ToLocalChecked(), kv::rocks::v8export());
}

NODE_MODULE(kv, init);
