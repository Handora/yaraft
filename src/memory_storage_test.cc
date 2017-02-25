// Copyright 2017 The etcd Authors
// Copyright 2017 Wu Tao
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <memory>

#include "memory_storage.h"

#include <gtest/gtest.h>

using namespace yaraft;

pb::Entry pbEntry(uint64_t index, uint64_t term) {
  pb::Entry tmp;
  tmp.set_term(term);
  tmp.set_index(index);
  return tmp;
}

using EntryVec = Storage::EntryVec;
using MemStoreUptr = std::unique_ptr<MemoryStorage>;

EntryVec& operator<<(EntryVec& v, const pb::Entry& e) {
  v.push_back(e);
  return v;
}

EntryVec& operator<<(EntryVec& v, const EntryVec& v2) {
  for (const auto& e : v2)
    v << e;
  return v;
}

EntryVec operator+(EntryVec v, pb::Entry e) {
  v.push_back(e);
  return v;
}

EntryVec operator+(pb::Entry e1, pb::Entry e2) {
  EntryVec v;
  v.push_back(e1);
  v.push_back(e2);
  return v;
}

inline bool operator==(pb::Entry e1, pb::Entry e2) {
  bool result = (e1.term() == e2.term()) && (e1.index() == e2.index());
  return result;
}

inline bool operator!=(pb::Entry e1, pb::Entry e2) {
  return !(e1 == e2);
}

inline bool operator==(EntryVec v1, EntryVec v2) {
  if (v1.size() != v2.size())
    return false;
  auto it1 = v1.begin();
  auto it2 = v2.begin();
  while (it1 != v1.end()) {
    if (*it1++ != *it2++)
      return false;
  }
  return true;
}

inline std::ostream& operator<<(std::ostream& os, const pb::Entry& e) {
  return os << "{Index: " << e.index() << ", Term: " << e.term() << "}";
}

inline std::ostream& operator<<(std::ostream& os, const EntryVec& v) {
  os << "Size of entry vec: " << v.size() << ". | ";
  for (const auto& e : v) {
    os << e << " | ";
  }
  return os << "\n";
}

TEST(MemoryStorage, Term) {
  struct TestData {
    uint64_t i;

    Error::ErrorCodes werr;
    uint64_t wterm;
  } tests[] = {{2, Error::LogCompacted, 0},
               {3, Error::LogCompacted, 0},
               {4, Error::OK, 4},
               {5, Error::OK, 5},
               {6, Error::OutOfBound, 0}};

  for (auto t : tests) {
    MemStoreUptr storage(MemoryStorage::TEST_Empty());
    storage->TEST_Entries() << pbEntry(3, 3) << pbEntry(4, 4) << pbEntry(5, 5);
    auto result = storage->Term(t.i);
    ASSERT_EQ(result.GetStatus().Code(), t.werr);

    if (result.OK())
      ASSERT_EQ(result.GetValue(), t.wterm);
  }
}

TEST(MemoryStorage, Compact) {
  struct TestData {
    uint64_t i;

    Error::ErrorCodes werr;
    uint64_t windex;
    uint64_t wterm;
    int wlen;
  } tests[] = {{2, Error::LogCompacted, 3, 3, 3},
               {3, Error::LogCompacted, 3, 3, 3},
               {4, Error::OK, 4, 4, 2},
               {5, Error::OK, 5, 5, 1}};

  for (auto t : tests) {
    MemStoreUptr storage(MemoryStorage::TEST_Empty());
    storage->TEST_Entries() << pbEntry(3, 3) << pbEntry(4, 4) << pbEntry(5, 5);
    auto status = storage->Compact(t.i);
    ASSERT_EQ(status.Code(), t.werr);
    ASSERT_EQ(storage->TEST_Entries()[0].index(), t.windex);
    ASSERT_EQ(storage->TEST_Entries()[0].term(), t.wterm);
  }
}

TEST(MemoryStorage, Entries) {
  auto maxUInt64 = std::numeric_limits<uint64_t>::max();
  auto ents = pbEntry(3, 3) + pbEntry(4, 4) + pbEntry(5, 5) + pbEntry(6, 6);

  struct TestData {
    uint64_t lo, hi, maxSize;

    Error::ErrorCodes werr;
    EntryVec went;
  } tests[] = {
      {2, 6, maxUInt64, Error::LogCompacted, EntryVec()},
      {3, 4, maxUInt64, Error::LogCompacted, EntryVec()},
      {4, 5, maxUInt64, Error::OK, EntryVec() + pbEntry(4, 4)},
      {4, 6, maxUInt64, Error::OK, pbEntry(4, 4) + pbEntry(5, 5)},
      {4, 7, maxUInt64, Error::OK, pbEntry(4, 4) + pbEntry(5, 5) + pbEntry(6, 6)},

      {4, 7, uint64_t(ents[0].ByteSize() + ents[1].ByteSize()), Error::OK,
       pbEntry(4, 4) + pbEntry(5, 5)},
      {4, 7, uint64_t(ents[0].ByteSize() + ents[1].ByteSize() + ents[2].ByteSize() / 2), Error::OK,
       pbEntry(4, 4) + pbEntry(5, 5)},
      {4, 7, uint64_t(ents[0].ByteSize() + ents[1].ByteSize() + ents[2].ByteSize() - 1), Error::OK,
       pbEntry(4, 4) + pbEntry(5, 5)},
      {4, 7, uint64_t(ents[0].ByteSize() + ents[1].ByteSize() + ents[2].ByteSize()), Error::OK,
       pbEntry(4, 4) + pbEntry(5, 5) + pbEntry(6, 6)},
  };
  for (auto t : tests) {
    MemStoreUptr storage(MemoryStorage::TEST_Empty());
    storage->TEST_Entries() << ents;
    auto status = storage->Entries(t.lo, t.hi, t.maxSize);
    ASSERT_EQ(status.GetStatus().Code(), t.werr);
    if (status.OK()) {
      Storage::EntryVec vec(status.GetValue().begin(), status.GetValue().end());
      ASSERT_TRUE(vec == t.went) << vec << t.went;
    }
  }
}
