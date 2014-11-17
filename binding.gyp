{
  "targets": [
    {
      "target_name": "kv",
	  "include_dirs" : [
        "<!(node -e \"require('nan')\")",
        "3rd-party/include"
      ],
      "dependencies": [
        "<(module_root_dir)/3rd-party/leveldb/leveldb.gyp:leveldb",
        "<(module_root_dir)/3rd-party/leveldb/rocksdb.gyp:rocksdb",
        "<(module_root_dir)/3rd-party/liblmdb/lmdb.gyp:lmdb"
      ],
      "sources": [
        "src/node-binding.cpp",
        "src/kv-types.cpp",
        "src/lmdb/setup-lmdb.cpp",
        "src/lmdb/env-lmdb.cpp",
        "src/lmdb/db-lmdb.cpp",
        "src/lmdb/cursor-lmdb.cpp",
        "src/lmdb/txn-lmdb.cpp",
        "src/leveldb/setup-lvl.cpp",
        "src/leveldb/db-lvl.cpp",
        "src/leveldb/cursor-lvl.cpp",
        "src/leveldb/batch-lvl.cpp"
      ],
      "conditions": [
        [
          "OS == 'win'", {
            "defines": [
              "_HAS_EXCEPTIONS=0"
            ]
          }
        ],
        [
          "OS=='linux'", {
            "cflags_cc": [ "-std=c++0x" ]
          }
        ]
      ]
    }
  ]
}