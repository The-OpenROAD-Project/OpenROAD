///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Google LLC
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <fstream>

#include "env.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "helper.h"
#include "odb/db.h"

namespace odb {
namespace {

constexpr char kPartition1[] = "CHAIN_1_FALLING_1";
constexpr char kPartition2[] = "CHAIN_1_RISING_1";

template <class>
inline constexpr bool always_false_v = false;

std::string GetName(const std::variant<dbBTerm*, dbITerm*>& pin)
{
  return std::visit(
      [](auto&& pin) {
        using T = std::decay_t<decltype(pin)>;
        if constexpr (std::is_same_v<T, dbBTerm*>) {
          return pin->getName();
        } else if constexpr (std::is_same_v<T, dbITerm*>) {
          return pin->getMTerm()->getName();
        } else {
          static_assert(always_false_v<T>, "non-exhaustive visitor");
        }
      },
      pin);
}

class TestScanChain : public testing::Test
{
 protected:
  TestScanChain()
  {
    db_ = create2LevetDbWithBTerms();
    block_ = db_->getChip()->getBlock();
    dft_ = block_->getDft();

    std::vector<std::string> instances_names = {"i1", "i2", "i3"};

    for (const auto& name : instances_names) {
      instances_.push_back(block_->findInst(name.c_str()));
    }
  }

  void SetUpTmpPath(const std::string& name)
  {
    tmp_path_ = testTmpPath("results", name);
  }

  // Writes a temporal DB and then tries to read the contents back to check if
  // the serialization is working as expected
  dbDatabase* writeReadDb()
  {
    writeDb();
    return readDb();
  }

  void writeDb()
  {
    std::ofstream write;
    write.exceptions(std::ifstream::failbit | std::ifstream::badbit
                     | std::ios::eofbit);
    write.open(tmp_path_, std::ios::binary);
    db_->write(write);
  }

  dbDatabase* readDb()
  {
    dbDatabase* db = dbDatabase::create();
    std::ifstream read;
    read.exceptions(std::ifstream::failbit | std::ifstream::badbit
                    | std::ios::eofbit);
    read.open(tmp_path_, std::ios::binary);
    db->read(read);
    return db;
  }

  dbDatabase* db_;
  dbBlock* block_;
  std::string tmp_path_;
  dbDft* dft_;
  std::vector<dbInst*> instances_;
};

TEST_F(TestScanChain, CreateScanChain)
{
  SetUpTmpPath("CreateScanChain");
  dbInst* inst = block_->findInst("i1");
  dbITerm* iterm = inst->findITerm("a");
  dbBTerm* bterm = block_->findBTerm("IN1");

  dbScanChain* scan_chain = dbScanChain::create(dft_);

  dbScanPartition* scan_partition = dbScanPartition::create(scan_chain);
  scan_partition->setName(kPartition1);
  dbScanList* scan_list = dbScanList::create(scan_partition);

  dbScanInst* scan_inst = scan_list->add(inst);
  scan_inst->setBits(1234);
  scan_inst->setAccessPins({.scan_in = iterm, .scan_out = bterm});

  //*****************************
  dbDatabase* db2 = writeReadDb();
  dbBlock* block2 = db2->getChip()->getBlock();
  dbDft* dft2 = block2->getDft();

  odb::dbSet<dbScanChain> scan_chains2 = dft2->getScanChains();
  EXPECT_THAT(scan_chains2.size(), 1);

  dbScanChain* scan_chain2 = *scan_chains2.begin();

  odb::dbSet<dbScanPartition> scan_partition2
      = scan_chain2->getScanPartitions();
  EXPECT_THAT(scan_partition2.size(), 1);
  EXPECT_THAT(scan_partition2.begin()->getName(), kPartition1);

  odb::dbSet<dbScanList> scan_lists2 = scan_partition2.begin()->getScanLists();
  EXPECT_THAT(scan_lists2.size(), 1);

  odb::dbSet<dbScanInst> scan_insts2 = scan_lists2.begin()->getScanInsts();
  EXPECT_THAT(scan_insts2.size(), 1);
}

TEST_F(TestScanChain, CreateScanChainWithPartition)
{
  SetUpTmpPath("CreateScanChainWithPartition");
  dbScanChain* scan_chain = dbScanChain::create(dft_);

  std::vector<dbInst*> instances_partition1 = {instances_[0], instances_[1]};
  std::vector<dbInst*> instances_partition2 = {instances_[2]};

  dbScanPartition* partition1 = dbScanPartition::create(scan_chain);
  dbScanPartition* partition2 = dbScanPartition::create(scan_chain);

  partition1->setName(kPartition1);
  partition2->setName(kPartition2);

  for (dbInst* inst : instances_partition1) {
    dbScanList* scan_list = dbScanList::create(partition1);
    dbScanInst* scan_inst = scan_list->add(inst);
    scan_inst->setBits(1);
    dbITerm* iterm = inst->findITerm("a");
    dbITerm* iterm2 = inst->findITerm("o");
    scan_inst->setAccessPins({.scan_in = iterm, .scan_out = iterm2});
  }

  for (dbInst* inst : instances_partition2) {
    dbScanList* scan_list = dbScanList::create(partition2);
    dbScanInst* scan_inst = scan_list->add(inst);
    scan_inst->setBits(1);
    dbITerm* iterm = inst->findITerm("a");
    dbITerm* iterm2 = inst->findITerm("o");
    scan_inst->setAccessPins({.scan_in = iterm, .scan_out = iterm2});
  }

  // 2 partitions, one for the first two instances and one for the last one
  //
  // Partition 1: [i1, i2]
  // Partition 2: [i3]

  //*****************************
  dbDatabase* db2 = writeReadDb();

  dbBlock* block2 = db2->getChip()->getBlock();
  dbDft* dft2 = block2->getDft();

  dbSet<dbScanChain> scan_chains2 = dft2->getScanChains();
  EXPECT_THAT(scan_chains2.size(), 1);

  dbSet<dbScanPartition> scan_partitions2
      = scan_chains2.begin()->getScanPartitions();
  EXPECT_THAT(scan_partitions2.size(), 2);

  auto iterator = scan_partitions2.begin();

  dbScanPartition* partition12 = *iterator;
  ++iterator;
  dbScanPartition* partition22 = *iterator;

  EXPECT_THAT(partition12->getName(), kPartition1);
  EXPECT_THAT(partition22->getName(), kPartition2);

  // check the created instances

  dbSet<dbScanList> scan_lists12 = partition12->getScanLists();
  dbSet<dbScanList> scan_lists22 = partition22->getScanLists();

  int i = 0;
  for (dbScanList* scan_list : scan_lists12) {
    for (dbScanInst* scan_inst : scan_list->getScanInsts()) {
      const dbScanInst::AccessPins& access_pins = scan_inst->getAccessPins();
      EXPECT_THAT(GetName(access_pins.scan_in), "a");
      EXPECT_THAT(GetName(access_pins.scan_out), "o");
      EXPECT_THAT(instances_[i]->getName(), scan_inst->getInst()->getName());
      ++i;
    }
  }

  for (dbScanList* scan_list : scan_lists22) {
    for (dbScanInst* scan_inst : scan_list->getScanInsts()) {
      const dbScanInst::AccessPins& access_pins = scan_inst->getAccessPins();
      EXPECT_THAT(GetName(access_pins.scan_in), "a");
      EXPECT_THAT(GetName(access_pins.scan_out), "o");
      EXPECT_THAT(instances_[i]->getName(), scan_inst->getInst()->getName());
      ++i;
    }
  }
}

}  // namespace
}  // namespace odb
