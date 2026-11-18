// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <fstream>
#include <iterator>
#include <string>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "tst/db_fixture.h"
#include "upf/upf.h"

namespace upf {
namespace {

class PowerSwitch : public tst::DbFixture
{
 protected:
  void SetUp() override
  {
    auto* tech = odb::dbTech::create(db_.get(), "tech");
    auto* chip = odb::dbChip::create(db_.get(), tech);
    block_ = odb::dbBlock::create(chip, "top");
    domain_ = odb::dbPowerDomain::create(block_, "PD_TOP");
    domain_->addElement(".");
    auto* lib = odb::dbLib::create(db_.get(), "lib", tech);
    master_ = odb::dbMaster::create(lib, "switch_cell");
    term_ = odb::dbMTerm::create(
        master_, "VIN", odb::dbIoType::INPUT, odb::dbSigType::POWER);
  }

  odb::dbPowerSwitch* makeSwitch(const char* name)
  {
    auto* ps = odb::dbPowerSwitch::create(block_, name);
    ps->setPowerDomain(domain_);
    domain_->addPowerSwitch(ps);
    ps->addInSupplyPort("vin", "VDD");
    ps->setOutSupplyPort("vout", "VDD_SW");
    ps->addControlPort("sleep", "shutdown");
    ps->addOnState("ON", "vin", "!sleep");
    return ps;
  }

  odb::dbBlock* block_;
  odb::dbPowerDomain* domain_;
  odb::dbMaster* master_;
  odb::dbMTerm* term_;
};

TEST_F(PowerSwitch, UnmappedThenMapped)
{
  auto* ps = makeSwitch("psw");
  EXPECT_EQ(ps->getLibCell(), nullptr);
  EXPECT_TRUE(ps->getPortMap().empty());

  ps->setLibCell(master_);
  ps->addPortMap("vin", term_);
  EXPECT_EQ(ps->getLibCell(), master_);
  ASSERT_EQ(ps->getPortMap().size(), 1u);
  EXPECT_EQ(ps->getPortMap().at("vin"), term_);
}

TEST_F(PowerSwitch, WriteWithUnmappedSwitch)
{
  makeSwitch("unmapped");
  auto* mapped = makeSwitch("mapped");
  mapped->setLibCell(master_);
  mapped->addPortMap("vin", term_);

  const std::string path = testing::TempDir() + "/power_switch.upf";
  write_upf(getLogger(), block_, path);
  std::ifstream file(path);
  ASSERT_TRUE(file.is_open());
  const std::string upf{std::istreambuf_iterator<char>(file),
                        std::istreambuf_iterator<char>()};
  EXPECT_NE(upf.find("create_power_switch unmapped "), std::string::npos);
  EXPECT_NE(upf.find("create_power_switch mapped "), std::string::npos);
  EXPECT_EQ(upf.find("map_power_switch unmapped"), std::string::npos);
  EXPECT_NE(upf.find("map_power_switch mapped "), std::string::npos);
  EXPECT_NE(upf.find("-lib_cells {switch_cell}"), std::string::npos);
  EXPECT_NE(upf.find("-port_map {{{vin} {VIN}}}"), std::string::npos);
}

}  // namespace
}  // namespace upf
