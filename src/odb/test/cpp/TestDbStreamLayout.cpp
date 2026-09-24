// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "helper.h"
#include "odb/db.h"

namespace odb {
namespace {

struct SlotRun
{
  std::string table;
  uint64_t offset;
  uint64_t length;
  uint64_t count;
};

class TestDbStreamLayout : public SimpleDbFixture
{
 protected:
  TestDbStreamLayout()
  {
    create2LevetDbWithBTerms();
    std::ostringstream file;
    std::ostringstream layout;
    db_->write(file, layout);
    bytes_ = file.str();
    parse(layout.str());
  }

  void parse(const std::string& text)
  {
    std::istringstream in(text);
    std::string word;
    int version = 0;
    in >> word >> version;
    ASSERT_EQ(word, "odb-layout");
    ASSERT_EQ(version, 1);
    while (in >> word) {
      if (word == "size") {
        in >> size_;
        break;
      }
      SlotRun run{.table = word, .offset = 0, .length = 0, .count = 0};
      in >> run.offset >> run.length >> run.count;
      runs_.push_back(run);
    }
  }

  // Allocated slots per table, read from the file through the layout.
  std::map<std::string, uint64_t> allocatedSlots() const
  {
    std::map<std::string, uint64_t> allocated;
    for (const SlotRun& run : runs_) {
      for (uint64_t i = 0; i < run.count; ++i) {
        allocated[run.table] += bytes_[run.offset + (i * run.length)] == 1;
      }
    }
    return allocated;
  }

  std::string bytes_;
  std::vector<SlotRun> runs_;
  uint64_t size_ = 0;
};

// Asking for a layout leaves the database bytes as they are.
TEST_F(TestDbStreamLayout, SameBytesAsWrite)
{
  std::ostringstream file;
  db_->write(file);
  EXPECT_EQ(file.str(), bytes_);
}

// The runs lie inside the file, in order, without overlapping, and each
// slot starts with its allocated flag.
TEST_F(TestDbStreamLayout, RunsDescribeTheFile)
{
  EXPECT_EQ(size_, bytes_.size());
  ASSERT_FALSE(runs_.empty());
  uint64_t end = 0;
  for (const SlotRun& run : runs_) {
    EXPECT_GE(run.offset, end) << run.table;
    EXPECT_GT(run.length, 0) << run.table;
    EXPECT_GT(run.count, 0) << run.table;
    end = run.offset + (run.length * run.count);
    ASSERT_LE(end, bytes_.size()) << run.table;
    for (uint64_t i = 0; i < run.count; ++i) {
      const char flag = bytes_[run.offset + (i * run.length)];
      EXPECT_TRUE(flag == 0 || flag == 1) << run.table;
    }
  }
}

// Every object is found where the layout says its table's slots are.
TEST_F(TestDbStreamLayout, FindsEveryObject)
{
  dbBlock* block = db_->getChip()->getBlock();
  const std::map<std::string, uint64_t> allocated = allocatedSlots();
  EXPECT_EQ(allocated.at("dbInst"), block->getInsts().size());
  EXPECT_EQ(allocated.at("dbNet"), block->getNets().size());
  EXPECT_EQ(allocated.at("dbITerm"), block->getITerms().size());
  EXPECT_EQ(allocated.at("dbBTerm"), block->getBTerms().size());
}

}  // namespace
}  // namespace odb
