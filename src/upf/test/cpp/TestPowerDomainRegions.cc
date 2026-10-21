// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// Region/group construction from UPF power domains -- the intent that
// test/upf_aes.tcl used to check by diffing a 109k-line DEF against
// upf_aes.defok (see The-OpenROAD-Project/OpenROAD#11281).

#include <string>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "tst/nangate45_fixture.h"
#include "upf/upf.h"

namespace upf {
namespace {

using odb::dbGroupType;
using odb::dbInst;
using odb::dbModInst;
using odb::dbModule;
using odb::dbRegion;
using odb::dbRegionType;
using odb::Rect;

// mpd_aes.upf in miniature: a top domain plus one domain per subblock.
const Rect kArea1(30000, 30000, 650000, 490000);
const Rect kArea2(30000, 510000, 650000, 970000);

class PowerDomainRegions : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    dbModule* top = block_->getTopModule();
    dbModule* aes = dbModule::create(block_, "aes_cipher_top");
    dbModule* aes2 = dbModule::create(block_, "aes_cipher_top_2");
    dbModInst::create(top, aes, "u_aes_1");
    dbModInst::create(top, aes2, "u_aes_2");

    odb::dbMaster* inv = db_->findMaster("INV_X1");
    aes1_inst_ = makeLeaf(inv, "u_aes_1/i0", aes);
    aes2_inst_ = makeLeaf(inv, "u_aes_2/i0", aes2);
    top_inst_ = makeLeaf(inv, "i0", top);
  }

  dbInst* makeLeaf(odb::dbMaster* master, const char* name, dbModule* parent)
  {
    tst::InstOptions options;
    options.parent_module = parent;
    return makeInst(block_, master, name, options);
  }

  odb::dbPowerDomain* makeDomain(const std::string& name,
                                 const std::string& element)
  {
    EXPECT_TRUE(create_power_domain(getLogger(), block_, name));
    EXPECT_TRUE(update_power_domain(getLogger(), block_, name, element));
    return block_->findPowerDomain(name.c_str());
  }

  void makeAesDomains()
  {
    makeDomain("PD_TOP", ".");
    makeDomain("PD_AES_1", "u_aes_1");
    makeDomain("PD_AES_2", "u_aes_2");
    ASSERT_TRUE(set_domain_area(getLogger(), block_, "PD_AES_1", kArea1));
    ASSERT_TRUE(set_domain_area(getLogger(), block_, "PD_AES_2", kArea2));
  }

  bool evalUpf()
  {
    return eval_upf(getSta()->getDbNetwork(), getLogger(), block_);
  }

  // The single boundary of a region, which is what DEF's
  // "- <name> ( x1 y1 ) ( x2 y2 )" is written from.
  Rect boundary(dbRegion* region)
  {
    odb::dbSet<odb::dbBox> boxes = region->getBoundaries();
    EXPECT_EQ(boxes.size(), 1u);
    return (*boxes.begin())->getBox();
  }

  dbInst* aes1_inst_;
  dbInst* aes2_inst_;
  dbInst* top_inst_;
};

// One fence region per non-top domain, at the area given to set_domain_area.
TEST_F(PowerDomainRegions, RegionPerDomainWithArea)
{
  makeAesDomains();
  ASSERT_TRUE(evalUpf());

  // The top domain covers the whole block and gets no region of its own; a
  // region for it would fence every instance into it.
  EXPECT_EQ(block_->getRegions().size(), 2u);

  dbRegion* region1 = block_->findRegion("PD_AES_1");
  dbRegion* region2 = block_->findRegion("PD_AES_2");
  ASSERT_NE(region1, nullptr);
  ASSERT_NE(region2, nullptr);

  // EXCLUSIVE is what write_def emits as "+ TYPE FENCE"; a GUIDE region is
  // advisory and would let placement leak cells out of the domain.
  EXPECT_EQ(region1->getRegionType(), dbRegionType::EXCLUSIVE);
  EXPECT_EQ(region2->getRegionType(), dbRegionType::EXCLUSIVE);

  EXPECT_EQ(boundary(region1), kArea1);
  EXPECT_EQ(boundary(region2), kArea2);
}

// Each domain gets a POWER_DOMAIN group under its region, holding exactly the
// instances of the modules the domain lists as elements.
TEST_F(PowerDomainRegions, InstancesGroupedByModule)
{
  makeAesDomains();
  ASSERT_TRUE(evalUpf());

  odb::dbGroup* group1 = block_->findPowerDomain("PD_AES_1")->getGroup();
  odb::dbGroup* group2 = block_->findPowerDomain("PD_AES_2")->getGroup();
  ASSERT_NE(group1, nullptr);
  ASSERT_NE(group2, nullptr);

  EXPECT_EQ(group1->getType(), dbGroupType::POWER_DOMAIN);
  EXPECT_EQ(group1->getRegion(), block_->findRegion("PD_AES_1"));
  EXPECT_EQ(group2->getRegion(), block_->findRegion("PD_AES_2"));

  ASSERT_EQ(group1->getInsts().size(), 1u);
  EXPECT_EQ(*group1->getInsts().begin(), aes1_inst_);
  ASSERT_EQ(group2->getInsts().size(), 1u);
  EXPECT_EQ(*group2->getInsts().begin(), aes2_inst_);

  // An instance of the top module belongs to the top domain, which has no
  // group, so it stays unfenced.
  EXPECT_EQ(top_inst_->getGroup(), nullptr);
  EXPECT_EQ(block_->findPowerDomain("PD_TOP")->getGroup(), nullptr);
  EXPECT_TRUE(block_->findPowerDomain("PD_TOP")->isTop());
}

// Non-top domains hang off the top domain, which is what makes
// add_insts_to_group() willing to fence their instances.
TEST_F(PowerDomainRegions, DomainHierarchy)
{
  makeAesDomains();
  ASSERT_TRUE(evalUpf());

  odb::dbPowerDomain* top = block_->findPowerDomain("PD_TOP");
  EXPECT_EQ(block_->findPowerDomain("PD_AES_1")->getParent(), top);
  EXPECT_EQ(block_->findPowerDomain("PD_AES_2")->getParent(), top);
  EXPECT_EQ(top->getParent(), nullptr);
}

// A domain with no set_domain_area still gets a region, but an empty one:
// nothing constrains its instances.
TEST_F(PowerDomainRegions, DomainWithoutAreaHasEmptyRegion)
{
  makeDomain("PD_TOP", ".");
  makeDomain("PD_AES_1", "u_aes_1");
  makeDomain("PD_AES_2", "u_aes_2");
  ASSERT_TRUE(set_domain_area(getLogger(), block_, "PD_AES_1", kArea1));

  ASSERT_TRUE(evalUpf());

  dbRegion* region2 = block_->findRegion("PD_AES_2");
  ASSERT_NE(region2, nullptr);
  EXPECT_TRUE(region2->getBoundaries().empty());
  EXPECT_EQ(boundary(block_->findRegion("PD_AES_1")), kArea1);
}

// set_domain_area on an undeclared domain is a warning, not a crash, and
// leaves no region behind.
TEST_F(PowerDomainRegions, UnknownDomainArea)
{
  makeDomain("PD_TOP", ".");
  EXPECT_FALSE(set_domain_area(getLogger(), block_, "PD_NOPE", kArea1));
  ASSERT_TRUE(evalUpf());
  EXPECT_TRUE(block_->getRegions().empty());
}

}  // namespace
}  // namespace upf
