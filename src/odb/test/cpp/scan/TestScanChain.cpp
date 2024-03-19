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

#include "db.h"
#include "env.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "helper.h"

namespace odb {
namespace {

constexpr std::string_view kPartition1 = "CHAIN_1_FALLING_1";
constexpr std::string_view kPartition2 = "CHAIN_1_RISING_1";

template <class>
inline constexpr bool always_false_v = false;

std::string_view GetName(const std::variant<dbBTerm*, dbITerm*>& pin)
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
    tmp_path_ = testTmpPath("results", "TestScanChain");
    dft_ = block_->getDft();

    std::vector<std::string> instances_names = {"i1", "i2", "i3"};

    for (const auto& name : instances_names) {
      instances_.push_back(block_->findInst(name.c_str()));
    }
  }

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
  dbInst* inst = block_->findInst("i1");
  dbITerm* iterm = inst->findITerm("a");
  dbBTerm* bterm = block_->findBTerm("IN1");

  dbScanChain* scan_chain = dbScanChain::create(dft_);

  dbScanList* scan_list = dbScanList::create(scan_chain);

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

  odb::dbSet<dbScanList> scan_lists2 = scan_chain2->getScanLists();
  EXPECT_THAT(scan_lists2.size(), 1);

  odb::dbSet<dbScanInst> scan_insts2 = scan_lists2.begin()->getScanInsts();
  EXPECT_THAT(scan_insts2.size(), 1);
}

TEST_F(TestScanChain, CreateScanChainWithPartition)
{
  dbScanChain* scan_chain = dbScanChain::create(dft_);
  dbScanList* scan_list = dbScanList::create(scan_chain);


  for (dbInst* inst : instances_) {
    dbScanInst* scan_inst = scan_list->add(inst);
    scan_inst->setBits(1);
    dbITerm* iterm = inst->findITerm("a");
    dbITerm* iterm2 = inst->findITerm("o");
    scan_inst->setAccessPins({.scan_in = iterm, .scan_out = iterm2});
  }

  // 2 partitions, one for the first instance and a second one for the second
  // instance. The partition 1 start at a bterm (chain scan in) and ends in the
  // scan out of the first instance.
  //
  // The second partition starts at the scan in of the second instance and ends
  // in the bterm scan out of the chain. The second partition contains 2
  // elements.
  //
  // Partition 1: [chain scan_in, i1 scan_out]
  // Partition 2: [i2 scan_in, chain scan out]

  dbScanPartition* partition1 = dbScanPartition::create(scan_chain);
  dbScanPartition* partition2 = dbScanPartition::create(scan_chain);

  dbBTerm* chain_scan_in = block_->findBTerm("IN1");
  dbBTerm* chain_scan_out = block_->findBTerm("IN3");

  partition1->setStart(chain_scan_in);
  partition1->setStop(instances_[0]->findITerm("o"));
  partition1->setName(kPartition1);

  partition2->setStart(instances_[1]->findITerm("a"));
  partition2->setStop(chain_scan_out);
  partition2->setName(kPartition2);

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

  EXPECT_THAT(GetName(partition12->getStart()), "IN1");
  EXPECT_THAT(GetName(partition12->getStop()), "o");

  EXPECT_THAT(GetName(partition22->getStart()), "a");
  EXPECT_THAT(GetName(partition22->getStop()), "IN3");

  // check the created instances
  dbSet<dbScanList> scan_lists2 = scan_chains2.begin()->getScanLists();
  dbSet<dbScanInst> scan_insts2 = scan_lists2.begin()->getScanInsts();

  int i = 0;
  for (dbScanInst* scan_inst: scan_insts2) {
    const dbScanInst::AccessPins& access_pins = scan_inst->getAccessPins();
    EXPECT_THAT(GetName(access_pins.scan_in), "a");
    EXPECT_THAT(GetName(access_pins.scan_out), "o");
    EXPECT_THAT(instances_[i]->getName(), scan_inst->getInst()->getName());
    ++i;
  }
}


TEST_F(TestScanChain, CreateScanChainWithMultipleScanLists)
{
  dbScanChain* scan_chain = dbScanChain::create(dft_);
  dbScanList* scan_list1 = dbScanList::create(scan_chain);
  dbScanList* scan_list2 = dbScanList::create(scan_chain);

  scan_list1->add(instances_[0]);
  scan_list2->add(instances_[1]);
  scan_list2->add(instances_[2]);

  dbDatabase* db2 = writeReadDb();

  dbBlock* block2 = db2->getChip()->getBlock();
  dbDft* dft2 = block2->getDft();

  dbSet<dbScanChain> scan_chains2 = dft2->getScanChains();
  EXPECT_THAT(scan_chains2.size(), 1);
  // check the created instances
  dbSet<dbScanList> scan_lists2 = scan_chains2.begin()->getScanLists();
  EXPECT_THAT(scan_lists2.size(), 2);

  int i = 0;
  for (dbScanList* scan_list: scan_lists2) {
    for (dbScanInst* scan_inst: scan_list->getScanInsts()) {
      EXPECT_THAT(scan_inst->getInst()->getName(), instances_[i]->getName());
      ++i;
    }
  }

  auto it = scan_lists2.begin();
  dbScanList* scan_list21 = *it;
  ++it;
  dbScanList* scan_list22 = *it;

  EXPECT_THAT(scan_list21->getScanInsts().size(), 1);
  EXPECT_THAT(scan_list22->getScanInsts().size(), 2);
}

}  // namespace
}  // namespace odb
