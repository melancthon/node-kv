{'targets': [{
    'target_name': 'rocksdb'
  , 'variables': {
        'ldbversion': 'rocksdb-3.7'
    }
  , 'type': 'static_library'
		# Overcomes an issue with the linker and thin .a files on SmartOS
  , 'standalone_static_library': 1
  , 'dependencies': [
        '../snappy/snappy.gyp:snappy'
    ]
  , 'direct_dependent_settings': {
        'include_dirs': [
            'leveldb-<(ldbversion)/include/'
          , 'leveldb-<(ldbversion)/port/'
          , 'leveldb-<(ldbversion)/util'
          , 'leveldb-<(ldbversion)/'
        ]
    }
  , 'defines': [
        'SNAPPY=1'
    ]
  , 'include_dirs': [
        'leveldb-<(ldbversion)/'
      , 'leveldb-<(ldbversion)/include/'
    ]
  , 'conditions': [
        ['OS == "win"', {
            'include_dirs': [
                'leveldb-<(ldbversion)/port/win'
              , 'port-libuv-rocksdb/'
            ]
          , 'defines': [
                'LEVELDB_PLATFORM_UV=1'
              , 'NOMINMAX=1'
              , '_HAS_EXCEPTIONS=0'
            ]
          , 'sources': [
                'port-libuv-rocksdb/port_uv.cc'
              , 'port-libuv-rocksdb/env_win.cc'
              , 'port-libuv-rocksdb/win_logger.cc'
            ]
          , 'msvs_settings': {
                'VCCLCompilerTool': {
                    'RuntimeTypeInfo': 'true'
                  , 'EnableFunctionLevelLinking': 'true'
                  , 'ExceptionHandling': '2'
                  , 'DisableSpecificWarnings': [ '4200', '4355', '4530' ,'4267', '4244', '4717' ]
                }
            }
        }, { # OS != "win"
            'sources': [
                'leveldb-<(ldbversion)/port/port_posix.cc'
              , 'leveldb-<(ldbversion)/port/port_posix.h'
              , 'leveldb-<(ldbversion)/util/env_posix.cc'
            ]
          , 'defines': [
                'ROCKSDB_PLATFORM_POSIX=1'
            ]
          , 'cflags': [
                '-std=gnu++0x'
              , '-fno-omit-frame-pointer'
              , '-momit-leaf-frame-pointer'
              , '-Woverloaded-virtual'
              , '-Wno-ignored-qualifiers'
              , '-Wno-type-limits'
              , '-Wno-unused-variable'
              , '-Wno-format-security'
              , '-fPIC'
            ]
          , 'cflags!': [
                '-fno-exceptions'
              , '-fno-rtti'
            ]
          , 'cflags_cc!': [
                '-fno-exceptions'
              , '-fno-rtti'
            ]
        }]
      , ['OS != "win"' and 'OS != "freebsd"', {
            'cflags': [
                '-Wno-sign-compare'
              , '-Wno-unused-but-set-variable'
            ]
        }]
      , ['OS == "linux"', {
            'defines': [
                'OS_LINUX=1'
            ]
          , 'libraries': [
                '-lpthread'
            ]
          , 'ccflags': [
                '-pthread'
            ]
        }]
      , ['OS == "freebsd"', {
            'defines': [
                'OS_FREEBSD=1'
              , '_REENTRANT=1'
            ]
          , 'libraries': [
                '-lpthread'
            ]
          , 'ccflags': [
                '-pthread'
            ]
          , 'cflags': [
                '-Wno-sign-compare'
            ]
        }]
      , ['OS == "solaris"', {
            'defines': [
                'OS_SOLARIS=1'
              , '_REENTRANT=1'
            ]
          , 'libraries': [
                '-lrt'
              , '-lpthread'
            ]
          , 'ccflags': [
                '-pthread'
            ]
        }]
      , ['OS == "mac"', {
            'defines': [
                'OS_MACOSX=1'
            ]
          , 'xcode_settings': {
                'MACOSX_DEPLOYMENT_TARGET': '10.7'
              , 'GCC_ENABLE_CPP_RTTI': 'YES'
              , 'WARNING_CFLAGS': [
                    '-Wno-sign-compare'
                  , '-Wno-unused-variable'
                  , '-Wno-unused-function'
                  , '-Wno-ignored-qualifiers'
                ]
              , 'OTHER_CPLUSPLUSFLAGS': [
                    '-std=c++11'
                  , '-stdlib=libc++'
                ]
              , 'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
            }
        }]
    ]
  , 'sources': [
        'leveldb-<(ldbversion)/db/db_filesnapshot.cc'
      , 'leveldb-<(ldbversion)/db/db_impl_readonly.cc'
      , 'leveldb-<(ldbversion)/db/c.cc'
      , 'leveldb-<(ldbversion)/db/internal_stats.cc'
      , 'leveldb-<(ldbversion)/db/compaction_job.cc'
      , 'leveldb-<(ldbversion)/db/compaction_picker.cc'
      , 'leveldb-<(ldbversion)/db/wal_manager.cc'
      , 'leveldb-<(ldbversion)/db/column_family.cc'
      , 'leveldb-<(ldbversion)/db/merge_helper.cc'
      , 'leveldb-<(ldbversion)/db/log_reader.cc'
      , 'leveldb-<(ldbversion)/db/repair.cc'
      , 'leveldb-<(ldbversion)/db/table_cache.cc'
      , 'leveldb-<(ldbversion)/db/log_writer.cc'
      , 'leveldb-<(ldbversion)/db/dbformat.cc'
      , 'leveldb-<(ldbversion)/db/write_batch.cc'
      , 'leveldb-<(ldbversion)/db/memtable_list.cc'
      , 'leveldb-<(ldbversion)/db/db_impl.cc'
      , 'leveldb-<(ldbversion)/db/memtable.cc'
      , 'leveldb-<(ldbversion)/db/version_edit.cc'
      , 'leveldb-<(ldbversion)/db/version_set.cc'
      , 'leveldb-<(ldbversion)/db/builder.cc'
      , 'leveldb-<(ldbversion)/db/merge_operator.cc'
      , 'leveldb-<(ldbversion)/db/filename.cc'
      , 'leveldb-<(ldbversion)/db/file_indexer.cc'
      , 'leveldb-<(ldbversion)/db/db_iter.cc'
      , 'leveldb-<(ldbversion)/db/version_builder.cc'
      , 'leveldb-<(ldbversion)/db/flush_scheduler.cc'
      , 'leveldb-<(ldbversion)/db/compaction.cc'
      , 'leveldb-<(ldbversion)/db/write_controller.cc'
      , 'leveldb-<(ldbversion)/db/write_thread.cc'
      , 'leveldb-<(ldbversion)/db/forward_iterator.cc'
      , 'leveldb-<(ldbversion)/db/flush_job.cc'
      , 'leveldb-<(ldbversion)/db/transaction_log_impl.cc'
      , 'leveldb-<(ldbversion)/db/table_properties_collector.cc'
      , 'leveldb-<(ldbversion)/helpers/memenv/memenv.cc'
      , 'leveldb-<(ldbversion)/port/stack_trace.cc'
      , 'leveldb-<(ldbversion)/table/merger.cc'
      , 'leveldb-<(ldbversion)/table/block_based_filter_block.cc'
      , 'leveldb-<(ldbversion)/table/block_based_table_factory.cc'
      , 'leveldb-<(ldbversion)/table/block_based_table_reader.cc'
      , 'leveldb-<(ldbversion)/table/block_builder.cc'
      , 'leveldb-<(ldbversion)/table/flush_block_policy.cc'
      , 'leveldb-<(ldbversion)/table/iterator.cc'
      , 'leveldb-<(ldbversion)/table/format.cc'
      , 'leveldb-<(ldbversion)/table/block.cc'
      , 'leveldb-<(ldbversion)/table/two_level_iterator.cc'
      , 'leveldb-<(ldbversion)/table/block_based_table_builder.cc'
      , 'leveldb-<(ldbversion)/table/bloom_block.cc'
      , 'leveldb-<(ldbversion)/table/block_prefix_index.cc'
      , 'leveldb-<(ldbversion)/table/meta_blocks.cc'
      , 'leveldb-<(ldbversion)/table/plain_table_key_coding.cc'
      , 'leveldb-<(ldbversion)/table/plain_table_reader.cc'
      , 'leveldb-<(ldbversion)/table/get_context.cc'
      , 'leveldb-<(ldbversion)/table/full_filter_block.cc'
      , 'leveldb-<(ldbversion)/table/table_properties.cc'
      , 'leveldb-<(ldbversion)/table/plain_table_builder.cc'
      , 'leveldb-<(ldbversion)/table/plain_table_factory.cc'
      , 'leveldb-<(ldbversion)/table/plain_table_index.cc'
      , 'leveldb-<(ldbversion)/table/block_hash_index.cc'
      , 'leveldb-<(ldbversion)/util/hash_linklist_rep.cc'
      , 'leveldb-<(ldbversion)/util/build_version.cc'
      , 'leveldb-<(ldbversion)/util/mutable_cf_options.cc'
      , 'leveldb-<(ldbversion)/util/log_buffer.cc'
      , 'leveldb-<(ldbversion)/util/bloom.cc'
      , 'leveldb-<(ldbversion)/util/arena.cc'
      , 'leveldb-<(ldbversion)/util/iostats_context.cc'
      , 'leveldb-<(ldbversion)/util/db_info_dumper.cc'
      , 'leveldb-<(ldbversion)/util/dynamic_bloom.cc'
      , 'leveldb-<(ldbversion)/util/options_helper.cc'
      , 'leveldb-<(ldbversion)/util/thread_local.cc'
      , 'leveldb-<(ldbversion)/util/sync_point.cc'
      , 'leveldb-<(ldbversion)/util/xxhash.cc'
      , 'leveldb-<(ldbversion)/util/slice.cc'
      , 'leveldb-<(ldbversion)/util/vectorrep.cc'
      , 'leveldb-<(ldbversion)/util/env.cc'
      , 'leveldb-<(ldbversion)/util/statistics.cc'
      , 'leveldb-<(ldbversion)/util/histogram.cc'
      , 'leveldb-<(ldbversion)/util/coding.cc'
      , 'leveldb-<(ldbversion)/util/perf_context.cc'
      , 'leveldb-<(ldbversion)/util/hash.cc'
      , 'leveldb-<(ldbversion)/util/logging.cc'
      , 'leveldb-<(ldbversion)/util/hash_skiplist_rep.cc'
      , 'leveldb-<(ldbversion)/util/auto_roll_logger.cc'
      , 'leveldb-<(ldbversion)/util/blob_store.cc'
      , 'leveldb-<(ldbversion)/util/filter_policy.cc'
      , 'leveldb-<(ldbversion)/util/murmurhash.cc'
      , 'leveldb-<(ldbversion)/util/cache.cc'
      , 'leveldb-<(ldbversion)/util/comparator.cc'
      , 'leveldb-<(ldbversion)/util/env_hdfs.cc'
      , 'leveldb-<(ldbversion)/util/skiplistrep.cc'
      , 'leveldb-<(ldbversion)/util/status.cc'
      , 'leveldb-<(ldbversion)/util/options.cc'
      , 'leveldb-<(ldbversion)/util/crc32c.cc'
      , 'leveldb-<(ldbversion)/util/string_util.cc'
    ]
}]}
