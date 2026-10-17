// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <exception>
#include <fstream>
#include <ios>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "helper.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"

namespace odb {
namespace {

std::vector<dbInst*> toVector(dbSet<dbInst> insts)
{
  return {insts.begin(), insts.end()};
}

// A bump master has exactly one pin.
dbMaster* createBumpMaster(dbLib* lib, const char* name)
{
  dbMaster* master = dbMaster::create(lib, name);
  master->setWidth(10);
  master->setHeight(10);
  master->setType(dbMasterType::COVER_BUMP);
  dbMTerm::create(master, "PAD", dbIoType::INOUT, dbSigType::SIGNAL);
  master->setFrozen();
  return master;
}

// A DIE chip with three regions and a few bump cells, plus a second DIE
// chip used as the "foreign" chip.
struct ChipBumpsFixture : public SimpleDbFixture
{
  ChipBumpsFixture()
  {
    dbDatabase* db = db_.get();
    tech_ = dbTech::create(db, "tech");
    layer_ = dbTechLayer::create(tech_, "L1", dbTechLayerType::MASTERSLICE);
    lib_ = dbLib::create(db, "lib", tech_, ',');
    bump_master_ = createBumpMaster(lib_, "bump");

    chip_ = dbChip::create(db, tech_, "die");
    block_ = dbBlock::create(chip_, "die_block");
    r1_ = dbChipRegion::create(chip_, "R1", dbChipRegion::Side::FRONT, layer_);
    r2_ = dbChipRegion::create(chip_, "R2", dbChipRegion::Side::BACK, layer_);
    r3_ = dbChipRegion::create(
        chip_, "R3", dbChipRegion::Side::INTERNAL, nullptr);

    other_chip_ = dbChip::create(db, tech_, "other_die");
    dbBlock::create(other_chip_, "other_block");
    other_region_ = dbChipRegion::create(
        other_chip_, "R1", dbChipRegion::Side::FRONT, layer_);
  }

  dbInst* makeBump(const char* name)
  {
    return dbInst::create(block_, bump_master_, name);
  }

  dbTech* tech_ = nullptr;
  dbTechLayer* layer_ = nullptr;
  dbLib* lib_ = nullptr;
  dbMaster* bump_master_ = nullptr;
  dbChip* chip_ = nullptr;
  dbBlock* block_ = nullptr;
  dbChipRegion* r1_ = nullptr;
  dbChipRegion* r2_ = nullptr;
  dbChipRegion* r3_ = nullptr;
  dbChip* other_chip_ = nullptr;
  dbChipRegion* other_region_ = nullptr;
};

TEST_F(ChipBumpsFixture, get_set_clear)
{
  dbInst* b = makeBump("b");
  EXPECT_EQ(b->getChipRegion(), nullptr);

  b->setChipRegion(r1_);
  EXPECT_EQ(b->getChipRegion(), r1_);
  EXPECT_EQ(toVector(r1_->getBumps()), std::vector<dbInst*>{b});

  // Same region again is a no-op.
  b->setChipRegion(r1_);
  EXPECT_EQ(r1_->getBumps().size(), 1);

  // Move to another region.
  b->setChipRegion(r2_);
  EXPECT_EQ(b->getChipRegion(), r2_);
  EXPECT_TRUE(r1_->getBumps().empty());
  EXPECT_EQ(toVector(r2_->getBumps()), std::vector<dbInst*>{b});

  b->setChipRegion(nullptr);
  EXPECT_EQ(b->getChipRegion(), nullptr);
  EXPECT_TRUE(r2_->getBumps().empty());
}

TEST_F(ChipBumpsFixture, foreign_region_rejected)
{
  dbInst* b = makeBump("b");
  EXPECT_THROW(b->setChipRegion(other_region_), std::exception);
  EXPECT_EQ(b->getChipRegion(), nullptr);
  EXPECT_TRUE(other_region_->getBumps().empty());
}

TEST_F(ChipBumpsFixture, chip_bump_guard)
{
  dbInst* b = makeBump("b");
  ASSERT_NE(dbChipBump::create(r1_, b), nullptr);
  EXPECT_EQ(b->getChipRegion(), r1_);

  // The region of an inst with a chip bump cannot change or be cleared.
  EXPECT_THROW(b->setChipRegion(r2_), std::exception);
  EXPECT_THROW(b->setChipRegion(nullptr), std::exception);
  EXPECT_EQ(b->getChipRegion(), r1_);
  EXPECT_EQ(b->getChipBump()->getChipRegion(), r1_);

  // Setting the same region is still allowed.
  b->setChipRegion(r1_);
  EXPECT_EQ(toVector(r1_->getBumps()), std::vector<dbInst*>{b});
}

TEST_F(ChipBumpsFixture, bump_master_needs_one_pin)
{
  dbMaster* no_pin = dbMaster::create(lib_, "no_pin");
  no_pin->setType(dbMasterType::COVER_BUMP);
  no_pin->setFrozen();
  dbMaster* two_pins = createMaster1X1(lib_, "two_pins", 10, 10, "in", "out");
  dbInst* a = dbInst::create(block_, no_pin, "a");
  dbInst* b = dbInst::create(block_, two_pins, "b");

  EXPECT_THROW(a->setChipRegion(r1_), std::exception);
  EXPECT_THROW(b->setChipRegion(r1_), std::exception);
  EXPECT_THROW(dbChipBump::create(r1_, a), std::exception);
  EXPECT_THROW(dbChipBump::create(r1_, b), std::exception);
  EXPECT_EQ(a->getChipRegion(), nullptr);
  EXPECT_EQ(b->getChipRegion(), nullptr);
  EXPECT_EQ(a->getChipBump(), nullptr);
  EXPECT_EQ(b->getChipBump(), nullptr);
  EXPECT_TRUE(r1_->getBumps().empty());
  EXPECT_TRUE(r1_->getChipBumps().empty());

  // A rejected add leaves the region's other bumps alone.
  dbInst* c = makeBump("c");
  c->setChipRegion(r2_);
  EXPECT_THROW(b->setChipRegion(r2_), std::exception);
  EXPECT_EQ(toVector(r2_->getBumps()), std::vector<dbInst*>{c});
}

TEST_F(ChipBumpsFixture, chip_bump_create_on_linked_inst)
{
  dbInst* a = makeBump("a");
  dbInst* b = makeBump("b");
  dbInst* c = makeBump("c");
  a->setChipRegion(r1_);
  b->setChipRegion(r1_);
  c->setChipRegion(r2_);
  EXPECT_EQ(toVector(r1_->getBumps()), (std::vector<dbInst*>{b, a}));

  // Same region: stays in place.
  ASSERT_NE(dbChipBump::create(r1_, a), nullptr);
  EXPECT_EQ(toVector(r1_->getBumps()), (std::vector<dbInst*>{b, a}));

  // Different region: moves to the front of the new region.
  ASSERT_NE(dbChipBump::create(r1_, c), nullptr);
  EXPECT_EQ(c->getChipRegion(), r1_);
  EXPECT_TRUE(r2_->getBumps().empty());
  EXPECT_EQ(toVector(r1_->getBumps()), (std::vector<dbInst*>{c, b, a}));
}

TEST_F(ChipBumpsFixture, newest_first_order)
{
  // Create in one order, add to the region in another.
  dbInst* b0 = makeBump("b0");
  dbInst* b1 = makeBump("b1");
  dbInst* b2 = makeBump("b2");
  dbInst* b3 = makeBump("b3");
  for (dbInst* inst : {b2, b0, b3, b1}) {
    inst->setChipRegion(r1_);
  }
  EXPECT_EQ(toVector(r1_->getBumps()), (std::vector<dbInst*>{b1, b3, b0, b2}));
  EXPECT_EQ(r1_->getBumps().size(), 4);

  // Removing from the middle keeps the order of the rest.
  b3->setChipRegion(nullptr);
  EXPECT_EQ(toVector(r1_->getBumps()), (std::vector<dbInst*>{b1, b0, b2}));
  // Removing head and tail.
  b1->setChipRegion(nullptr);
  b2->setChipRegion(nullptr);
  EXPECT_EQ(toVector(r1_->getBumps()), std::vector<dbInst*>{b0});
  // Re-adding prepends.
  b3->setChipRegion(r1_);
  EXPECT_EQ(toVector(r1_->getBumps()), (std::vector<dbInst*>{b3, b0}));
}

TEST_F(ChipBumpsFixture, reverse)
{
  std::vector<dbInst*> created;
  for (const char* name : {"b0", "b1", "b2"}) {
    dbInst* inst = makeBump(name);
    dbChipBump::create(r1_, inst);
    created.push_back(inst);
  }
  // Newest first, then creation order (the bump table order) once reversed.
  EXPECT_EQ(toVector(r1_->getBumps()),
            (std::vector<dbInst*>{created[2], created[1], created[0]}));
  r1_->getBumps().reverse();
  EXPECT_EQ(toVector(r1_->getBumps()), created);
  std::vector<dbInst*> from_bumps;
  for (dbChipBump* bump : r1_->getChipBumps()) {
    from_bumps.push_back(bump->getInst());
  }
  EXPECT_EQ(from_bumps, created);

  // Reversing an empty list is a no-op.
  r2_->getBumps().reverse();
  EXPECT_TRUE(r2_->getBumps().empty());
}

TEST_F(ChipBumpsFixture, unlink_on_destroy)
{
  dbInst* a = makeBump("a");
  dbInst* b = makeBump("b");
  dbInst* c = makeBump("c");
  for (dbInst* inst : {a, b, c}) {
    inst->setChipRegion(r1_);
  }
  // List is c, b, a.
  dbInst::destroy(b);
  EXPECT_EQ(toVector(r1_->getBumps()), (std::vector<dbInst*>{c, a}));
  dbInst::destroy(c);
  EXPECT_EQ(toVector(r1_->getBumps()), std::vector<dbInst*>{a});
  dbInst::destroy(a);
  EXPECT_TRUE(r1_->getBumps().empty());
  EXPECT_TRUE(chip_->getBumps().empty());

  // The region keeps working after its list was emptied.
  dbInst* d = makeBump("d");
  d->setChipRegion(r1_);
  EXPECT_EQ(toVector(r1_->getBumps()), std::vector<dbInst*>{d});
}

TEST_F(ChipBumpsFixture, inst_without_region_is_not_a_bump)
{
  dbInst* a = makeBump("a");
  dbInst* no_region = makeBump("no_region");
  a->setChipRegion(r2_);
  EXPECT_EQ(toVector(chip_->getBumps()), std::vector<dbInst*>{a});
  EXPECT_EQ(no_region->getBumpBTerm(), nullptr);
}

TEST_F(ChipBumpsFixture, bump_bterm_set_get_clear)
{
  dbInst* a = makeBump("a");
  dbBTerm* sig1 = dbBTerm::create(dbNet::create(block_, "SIG1"), "SIG1");
  EXPECT_EQ(a->getBumpBTerm(), nullptr);

  // Needs a chip region.
  EXPECT_THROW(a->setBumpBTerm(sig1), std::exception);
  EXPECT_EQ(a->getBumpBTerm(), nullptr);
  // Clearing is allowed without one.
  a->setBumpBTerm(nullptr);

  a->setChipRegion(r1_);
  a->setBumpBTerm(sig1);
  EXPECT_EQ(a->getBumpBTerm(), sig1);
  a->setBumpBTerm(nullptr);
  EXPECT_EQ(a->getBumpBTerm(), nullptr);
}

TEST_F(ChipBumpsFixture, bump_bterm_foreign_block)
{
  dbInst* a = makeBump("a");
  a->setChipRegion(r1_);
  dbBlock* other_block = other_chip_->getBlock();
  dbBTerm* other
      = dbBTerm::create(dbNet::create(other_block, "OTHER"), "OTHER");
  EXPECT_THROW(a->setBumpBTerm(other), std::exception);
  EXPECT_EQ(a->getBumpBTerm(), nullptr);
}

TEST_F(ChipBumpsFixture, bump_bterm_kept_on_region_move)
{
  dbInst* a = makeBump("a");
  dbBTerm* sig1 = dbBTerm::create(dbNet::create(block_, "SIG1"), "SIG1");
  a->setChipRegion(r1_);
  a->setBumpBTerm(sig1);
  a->setChipRegion(r2_);
  EXPECT_EQ(a->getBumpBTerm(), sig1);

  // The dbChipBump::create relink keeps it too.
  dbChipBump::create(r3_, a);
  EXPECT_EQ(a->getChipRegion(), r3_);
  EXPECT_EQ(a->getBumpBTerm(), sig1);
}

TEST_F(ChipBumpsFixture, bump_bterm_cleared_on_leaving_regions)
{
  dbInst* a = makeBump("a");
  dbBTerm* sig1 = dbBTerm::create(dbNet::create(block_, "SIG1"), "SIG1");
  a->setChipRegion(r1_);
  a->setBumpBTerm(sig1);
  a->setChipRegion(nullptr);
  EXPECT_EQ(a->getBumpBTerm(), nullptr);
  // Rejoining a region does not bring it back.
  a->setChipRegion(r1_);
  EXPECT_EQ(a->getBumpBTerm(), nullptr);
}

TEST_F(ChipBumpsFixture, bump_bterm_cleared_on_inst_destroy)
{
  dbInst* a = makeBump("a");
  dbBTerm* sig1 = dbBTerm::create(dbNet::create(block_, "SIG1"), "SIG1");
  a->setChipRegion(r1_);
  a->setBumpBTerm(sig1);
  dbInst::destroy(a);
  // A new inst reusing the slot starts unbound.
  dbInst* b = makeBump("b");
  b->setChipRegion(r1_);
  EXPECT_EQ(b->getBumpBTerm(), nullptr);
}

TEST_F(ChipBumpsFixture, bump_bterm_cleared_on_bterm_destroy)
{
  dbInst* a = makeBump("a");
  dbInst* b = makeBump("b");
  dbNet* net = dbNet::create(block_, "SIG1");
  dbBTerm* sig1 = dbBTerm::create(net, "SIG1");
  dbBTerm* sig2 = dbBTerm::create(net, "SIG2");
  a->setChipRegion(r1_);
  b->setChipRegion(r2_);
  a->setBumpBTerm(sig1);
  b->setBumpBTerm(sig2);
  dbBTerm::destroy(sig1);
  EXPECT_EQ(a->getBumpBTerm(), nullptr);
  EXPECT_EQ(b->getBumpBTerm(), sig2);
}

// Two ports on one net, each on its own bump: the binding is per bump.
TEST_F(ChipBumpsFixture, bump_bterm_two_ports_one_net)
{
  dbInst* a = makeBump("a");
  dbInst* b = makeBump("b");
  dbNet* net = dbNet::create(block_, "SIG");
  dbBTerm* in = dbBTerm::create(net, "IN");
  dbBTerm* out = dbBTerm::create(net, "OUT");
  a->setChipRegion(r1_);
  b->setChipRegion(r1_);
  a->findITerm("PAD")->connect(net);
  b->findITerm("PAD")->connect(net);
  a->setBumpBTerm(in);
  b->setBumpBTerm(out);
  EXPECT_EQ(a->getBumpBTerm(), in);
  EXPECT_EQ(b->getBumpBTerm(), out);
  EXPECT_EQ(a->findITerm("PAD")->getNet(), net);
  EXPECT_EQ(b->findITerm("PAD")->getNet(), net);
}

TEST_F(ChipBumpsFixture, chip_bump_bterm_goes_through_inst)
{
  dbInst* a = makeBump("a");
  dbBTerm* sig1 = dbBTerm::create(dbNet::create(block_, "SIG1"), "SIG1");
  dbBTerm* sig2 = dbBTerm::create(dbNet::create(block_, "SIG2"), "SIG2");
  dbChipBump* bump = dbChipBump::create(r1_, a);
  bump->setBTerm(sig1);
  EXPECT_EQ(a->getBumpBTerm(), sig1);
  EXPECT_EQ(bump->getBTerm(), sig1);
  EXPECT_EQ(sig1->getChipBump(), bump);
  // getBTerm answers from the inst binding.
  a->setBumpBTerm(sig2);
  EXPECT_EQ(bump->getBTerm(), sig2);
  a->setBumpBTerm(nullptr);
  EXPECT_EQ(bump->getBTerm(), nullptr);
}

TEST_F(ChipBumpsFixture, chip_bump_set_net_connects_pin)
{
  dbInst* a = makeBump("a");
  dbChipBump* bump = dbChipBump::create(r1_, a);
  dbITerm* pad = a->findITerm("PAD");
  EXPECT_EQ(bump->getNet(), nullptr);

  dbNet* net = dbNet::create(block_, "SIG1");
  bump->setNet(net);
  EXPECT_EQ(pad->getNet(), net);
  EXPECT_EQ(bump->getNet(), net);

  bump->setNet(nullptr);
  EXPECT_EQ(pad->getNet(), nullptr);
  EXPECT_EQ(bump->getNet(), nullptr);
}

TEST_F(ChipBumpsFixture, chip_bump_get_net_follows_pin)
{
  dbInst* a = makeBump("a");
  dbChipBump* bump = dbChipBump::create(r1_, a);
  dbNet* net1 = dbNet::create(block_, "SIG1");
  dbNet* net2 = dbNet::create(block_, "SIG2");
  bump->setNet(net1);
  a->findITerm("PAD")->connect(net2);
  EXPECT_EQ(bump->getNet(), net2);
  a->findITerm("PAD")->disconnect();
  EXPECT_EQ(bump->getNet(), nullptr);
}

TEST_F(ChipBumpsFixture, chip_iterator)
{
  // Empty: regions but no bumps.
  EXPECT_TRUE(chip_->getBumps().empty());
  EXPECT_EQ(chip_->getBumps().size(), 0);

  dbInst* a = makeBump("a");
  dbInst* b = makeBump("b");
  dbInst* c = makeBump("c");
  dbInst* d = makeBump("d");
  // R1 stays empty to exercise skipping an empty region at the start.
  c->setChipRegion(r3_);
  a->setChipRegion(r2_);
  d->setChipRegion(r3_);
  b->setChipRegion(r2_);
  // R2 is b, a; R3 is d, c.
  EXPECT_EQ(toVector(chip_->getBumps()), (std::vector<dbInst*>{b, a, d, c}));
  EXPECT_EQ(chip_->getBumps().size(), 4);

  // Empty region in the middle.
  a->setChipRegion(r1_);
  b->setChipRegion(r3_);
  EXPECT_TRUE(r2_->getBumps().empty());
  EXPECT_EQ(toVector(chip_->getBumps()), (std::vector<dbInst*>{a, b, d, c}));
}

TEST_F(ChipBumpsFixture, chip_iterator_empty_chips)
{
  // HIER chip: no block, no regions.
  dbChip* hier
      = dbChip::create(db_.get(), nullptr, "hier", dbChip::ChipType::HIER);
  EXPECT_TRUE(hier->getBumps().empty());

  // DIE chip with a block but no regions.
  dbChip* bare = dbChip::create(db_.get(), tech_, "bare");
  dbBlock::create(bare, "bare_block");
  EXPECT_TRUE(bare->getBumps().empty());

  // DIE chip with a region but no block.
  dbChip* blockless = dbChip::create(db_.get(), tech_, "blockless");
  dbChipRegion* region = dbChipRegion::create(
      blockless, "R1", dbChipRegion::Side::FRONT, layer_);
  EXPECT_TRUE(blockless->getBumps().empty());
  EXPECT_TRUE(region->getBumps().empty());
}

TEST_F(ChipBumpsFixture, destroy_top_block_clears_lists)
{
  dbInst* a = makeBump("a");
  a->setChipRegion(r1_);
  dbBlock::destroy(block_);
  EXPECT_TRUE(r1_->getBumps().empty());
  EXPECT_TRUE(chip_->getBumps().empty());
}

TEST_F(ChipBumpsFixture, write_read_round_trip)
{
  dbInst* a = makeBump("a");
  dbInst* b = makeBump("b");
  dbInst* c = makeBump("c");
  dbChipBump::create(r2_, b);
  a->setChipRegion(r2_);
  c->setChipRegion(r1_);
  dbNet* net = dbNet::create(block_, "SIG");
  a->setBumpBTerm(dbBTerm::create(net, "IN"));
  c->setBumpBTerm(dbBTerm::create(net, "OUT"));

  std::stringstream ss;
  db_->write(ss);
  std::unique_ptr<dbDatabase, void (*)(dbDatabase*)> db_in(dbDatabase::create(),
                                                           dbDatabase::destroy);
  db_in->setLogger(getLogger());
  db_in->read(ss);

  auto names = [](dbSet<dbInst> insts) {
    std::vector<std::string> out;
    for (dbInst* inst : insts) {
      out.push_back(inst->getName());
    }
    return out;
  };
  dbChip* chip = db_in->findChip("die");
  ASSERT_NE(chip, nullptr);
  dbChipRegion* r1 = chip->findChipRegion("R1");
  dbChipRegion* r2 = chip->findChipRegion("R2");
  EXPECT_EQ(names(r1->getBumps()), std::vector<std::string>{"c"});
  EXPECT_EQ(names(r2->getBumps()), (std::vector<std::string>{"a", "b"}));
  EXPECT_EQ(names(chip->getBumps()), (std::vector<std::string>{"c", "a", "b"}));
  dbBlock* block = chip->getBlock();
  EXPECT_EQ(block->findInst("a")->getChipRegion(), r2);
  EXPECT_EQ(block->findInst("a")->getBumpBTerm(), block->findBTerm("IN"));
  EXPECT_EQ(block->findInst("b")->getBumpBTerm(), nullptr);
  EXPECT_EQ(block->findInst("c")->getBumpBTerm(), block->findBTerm("OUT"));
}

// A regioned inst without a chip bump still unfolds, but has no folded
// chip bump inst.
TEST_F(SimpleDbFixture, unfolded_bump_without_chip_bump)
{
  dbDatabase* db = db_.get();
  // The first chip created is the top chip.
  dbChip* top = dbChip::create(db, nullptr, "top", dbChip::ChipType::HIER);
  dbTech* tech = dbTech::create(db, "tech");
  dbTechLayer* layer
      = dbTechLayer::create(tech, "L1", dbTechLayerType::MASTERSLICE);
  dbLib* lib = dbLib::create(db, "lib", tech, ',');
  dbMaster* master = createBumpMaster(lib, "bump");
  dbChip* die = dbChip::create(db, tech, "die");
  dbBlock* block = dbBlock::create(die, "die_block");
  dbChipRegion* region
      = dbChipRegion::create(die, "R1", dbChipRegion::Side::FRONT, layer);

  dbInst* real = dbInst::create(block, master, "real");
  dbChipBump::create(region, real);
  dbInst* plain = dbInst::create(block, master, "plain");
  plain->setChipRegion(region);
  EXPECT_EQ(toVector(region->getBumps()), (std::vector<dbInst*>{plain, real}));

  dbChipInst::create(top, die, "die_inst");
  db->constructUnfoldedModel();

  std::vector<dbUnfoldedChipBumpInst*> uf_bumps;
  for (dbUnfoldedChipBumpInst* bump : db->getUnfoldedChipBumpInsts()) {
    uf_bumps.push_back(bump);
  }
  ASSERT_EQ(uf_bumps.size(), 2);
  dbUnfoldedChipBumpInst* uf_real = nullptr;
  dbUnfoldedChipBumpInst* uf_plain = nullptr;
  for (dbUnfoldedChipBumpInst* bump : uf_bumps) {
    if (bump->getInst() == real) {
      uf_real = bump;
    } else if (bump->getInst() == plain) {
      uf_plain = bump;
    }
  }
  ASSERT_NE(uf_real, nullptr);
  ASSERT_NE(uf_plain, nullptr);
  ASSERT_NE(uf_real->getChipBumpInst(), nullptr);
  EXPECT_EQ(uf_real->getChipBumpInst()->getChipBump()->getInst(), real);
  EXPECT_EQ(uf_plain->getChipBumpInst(), nullptr);
}

// Database fixtures written by older OpenROAD versions.
struct OldDatabaseFixture : public SimpleDbFixture
{
  std::unique_ptr<dbDatabase, void (*)(dbDatabase*)> readOdb(
      const std::string& path)
  {
    std::unique_ptr<dbDatabase, void (*)(dbDatabase*)> db(dbDatabase::create(),
                                                          dbDatabase::destroy);
    db->setLogger(getLogger());
    std::ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit
                    | std::ios::eofbit);
    file.open(path, std::ios::binary);
    db->read(file);
    return db;
  }

  // Files under the repository's top-level test directory. Bazel resolves
  // them through runfiles; the CMake cpp_tests.tcl runner runs from
  // src/odb/test.
  std::string topTestFile(const std::string& name)
  {
    try {
      return getFilePath("_main/test/" + name);
    } catch (const std::runtime_error&) {
      return getFilePath("../../../test/" + name);
    }
  }
};

// Non-3D database at schema 120, inside the range where chip bumps exist.
TEST_F(OldDatabaseFixture, reads_gcd_sky130hd)
{
  auto db = readOdb(topTestFile("gcd_sky130hd.odb"));
  dbChip* chip = db->getChip();
  ASSERT_NE(chip, nullptr);
  EXPECT_NE(chip->getBlock(), nullptr);
  EXPECT_TRUE(chip->getBumps().empty());
}

// Non-3D database at the initial schema (57).
TEST_F(OldDatabaseFixture, reads_design_odb)
{
  auto db = readOdb(getFilePath("_main/src/odb/test/data/design.odb"));
  dbChip* chip = db->getChip();
  ASSERT_NE(chip, nullptr);
  EXPECT_TRUE(chip->getBumps().empty());
}

}  // namespace
}  // namespace odb
