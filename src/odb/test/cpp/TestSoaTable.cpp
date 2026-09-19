// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// What the field-major dbTable block format has to preserve.
//
// These assert the properties the format claims, not the bytes it currently
// produces: no golden file, nothing to regenerate when an unrelated change
// perturbs output. The claim is that writing a block field-major is a
// permutation of the same bytes, so a database that has been through it is
// indistinguishable from one that has not -- same ids, same connectivity,
// and, the subtle one, the same *next* id, because allocation order is the
// free list's order and the free list is part of what gets written.

#include <sstream>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "helper/helper.h"
#include "odb/db.h"
#include "utl/deleter.h"

namespace odb {
namespace {

class SoaTable : public SimpleDbFixture
{
 protected:
  void SetUp() override { create2LevetDbNoBTerms(); }

  std::string write(dbDatabase* db)
  {
    std::ostringstream out(std::ios::binary);
    db->write(out);
    return out.str();
  }

  // A fresh database holding what `bytes` describes. Kept alive by the
  // caller so ids can be compared against the original.
  utl::UniquePtrWithDeleter<dbDatabase> read(const std::string& bytes)
  {
    utl::UniquePtrWithDeleter<dbDatabase> db(dbDatabase::create(),
                                             &dbDatabase::destroy);
    db->setLogger(getLogger());
    std::istringstream in(bytes, std::ios::binary);
    db->read(in);
    return db;
  }

  static dbBlock* blockOf(dbDatabase* db) { return db->getChip()->getBlock(); }

  // Every iterm as (id, instance name, terminal name, net name). Enough to
  // catch an id that moved, a connection that changed, or an object that
  // came back as a different one.
  static std::vector<std::string> itermSummary(dbBlock* block)
  {
    std::vector<std::string> summary;
    for (dbITerm* iterm : block->getITerms()) {
      dbNet* net = iterm->getNet();
      summary.push_back(std::to_string(iterm->getId()) + " "
                        + iterm->getInst()->getName() + " "
                        + iterm->getMTerm()->getName() + " "
                        + (net ? net->getName() : "<none>"));
    }
    return summary;
  }

  // The ids a freshly created instance's iterms get. This is what free-list
  // order decides, and what a reload must not perturb.
  static std::vector<uint32_t> nextIds(dbDatabase* db, const char* name)
  {
    dbBlock* block = blockOf(db);
    dbMaster* master = db->findLib("lib1")->findMaster("and2");
    dbInst* inst = dbInst::create(block, master, name);
    std::vector<uint32_t> ids;
    for (dbITerm* iterm : inst->getITerms()) {
      ids.push_back(iterm->getId());
    }
    return ids;
  }
};

TEST_F(SoaTable, RoundTripPreservesIdsAndConnectivity)
{
  const std::vector<std::string> before = itermSummary(blockOf(getDb()));
  auto reloaded = read(write(getDb()));
  EXPECT_EQ(itermSummary(blockOf(reloaded.get())), before);
}

TEST_F(SoaTable, WritingWhatWasReadReproducesTheSameBytes)
{
  // A permutation is only lossless if it round-trips exactly. This is the
  // cheapest statement of that, and it covers every table at once.
  const std::string once = write(getDb());
  auto reloaded = read(once);
  EXPECT_EQ(write(reloaded.get()), once);
}

TEST_F(SoaTable, AllocationOrderSurvivesAReload)
{
  // Destroying an instance returns its iterms to the free list, so the next
  // allocation reuses those slots in the order the free list holds them.
  // If a reload rebuilt the list from the allocation bitmap instead of
  // reading it, ids would still be *valid* and the design would still be
  // correct -- and every later object would get a different id, which
  // reaches placement and routing through dbSet iteration order.
  dbBlock* block = blockOf(getDb());
  dbInst::destroy(block->findInst("i2"));

  const std::string bytes = write(getDb());
  const std::vector<uint32_t> without_reload = nextIds(getDb(), "i4");

  auto reloaded = read(bytes);
  const std::vector<uint32_t> after_reload = nextIds(reloaded.get(), "i4");

  EXPECT_EQ(after_reload, without_reload);
  EXPECT_FALSE(after_reload.empty());
}

// Records of two different lengths in one block -- the case that appears
// once pin access has run and an iterm carries an access point -- is not
// covered here. Building that state synthetically did not reproduce it:
// dbAccessPoint::create() followed by dbITerm::setAccessPoint() leaves
// dbITerm::getAccessPoints() empty, with or without the field-major layout,
// so the setup a test would rest on cannot be asserted. It is covered
// end-to-end instead, on a routed design where 71.8% of iterms carry an
// access point, by study/soa/roundtrip_gate.sh in the bazel-orfs study.

}  // namespace
}  // namespace odb
