// Copyright 2020 The Pigweed Authors
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#include "gtest/gtest.h"
#include "pw_kvs/crc16_checksum.h"
#include "pw_kvs/flash_partition_with_stats.h"
#include "pw_kvs/in_memory_fake_flash.h"
#include "pw_kvs/key_value_store.h"

namespace pw::kvs {
namespace {

using std::byte;

#ifndef PW_KVS_FUZZ_ITERATIONS
#define PW_KVS_FUZZ_ITERATIONS 2
#endif  // PW_KVS_FUZZ_ITERATIONS
constexpr int kFuzzIterations = PW_KVS_FUZZ_ITERATIONS;

constexpr size_t kMaxEntries = 256;
constexpr size_t kMaxUsableSectors = 256;

// 4 x 4k sectors, 16 byte alignment
FakeFlashBuffer<4 * 1024, 6> test_flash(16);

FlashPartitionWithStatsBuffer<kMaxEntries> test_partition(
    &test_flash, 0, test_flash.sector_count());

ChecksumCrc16 checksum;

class EmptyInitializedKvs : public ::testing::Test {
 protected:
  EmptyInitializedKvs()
      : kvs_(&test_partition, {.magic = 0xBAD'C0D3, .checksum = &checksum}) {
    test_partition.Erase(0, test_partition.sector_count());
    ASSERT_EQ(Status::OK, kvs_.Init());
  }

  KeyValueStoreBuffer<kMaxEntries, kMaxUsableSectors> kvs_;
};

TEST_F(EmptyInitializedKvs, Put_VaryingKeysAndValues) {
  char value[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"  // 52
      "34567890123";  // 64 (with final \0);
  static_assert(sizeof(value) == 64);

  test_partition.ResetCounters();

  for (int i = 0; i < kFuzzIterations; ++i) {
    for (unsigned key_size = 1; key_size < sizeof(value); ++key_size) {
      for (unsigned value_size = 0; value_size < sizeof(value); ++value_size) {
        ASSERT_EQ(Status::OK,
                  kvs_.Put(std::string_view(value, key_size),
                           as_bytes(span(value, value_size))));
      }
    }
  }

  test_partition.SaveStorageStats(kvs_, "fuzz Put_VaryingKeysAndValues");
}

}  // namespace
}  // namespace pw::kvs
