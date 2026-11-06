// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "sta/PortDirection.hh"
#include "tst/fixture.h"

namespace sta {
namespace {

class BTermDirectionTest : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    auto* tech = odb::dbTech::create(db_.get(), "tech");
    auto* chip = odb::dbChip::create(db_.get(), tech);
    block_ = odb::dbBlock::create(chip, "top");
    sta_->postReadDef(block_);
  }

  odb::dbBlock* block_ = nullptr;
};

TEST_F(BTermDirectionTest, SignalTypeChanges)
{
  auto* network = sta_->getDbNetwork();
  for (const auto io_type :
       {odb::dbIoType::INPUT, odb::dbIoType::OUTPUT, odb::dbIoType::INOUT}) {
    auto* net = odb::dbNet::create(block_, odb::dbIoType(io_type).getString());
    auto* bterm = odb::dbBTerm::create(net, net->getConstName());
    bterm->setIoType(io_type);
    auto* port = network->findPort(network->cell(network->topInstance()),
                                   bterm->getConstName());
    ASSERT_NE(port, nullptr);
    const auto* signal_direction = network->direction(port);
    for (const auto sig_type :
         {odb::dbSigType::POWER, odb::dbSigType::GROUND}) {
      bterm->setSigType(sig_type);
      EXPECT_EQ(network->direction(port),
                sig_type == odb::dbSigType::POWER ? PortDirection::power()
                                                  : PortDirection::ground());
      bterm->setSigType(odb::dbSigType::SIGNAL);
      EXPECT_EQ(network->direction(port), signal_direction);
    }
  }
}

}  // namespace
}  // namespace sta
