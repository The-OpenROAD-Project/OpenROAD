// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <initializer_list>

#include "Timing.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "tst/nangate45_fixture.h"

namespace wmk {
namespace {

class TestDriverHeadroom : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    readLiberty("_main/test/Nangate45/Nangate45_typ.lib");
    sta_->postReadDb(db_.get());
    network_ = sta_->getDbNetwork();
    auto* input = odb::dbNet::create(block_, "input");
    odb::dbBTerm::create(input, "input")->setIoType(odb::dbIoType::INPUT);
    auto* driver
        = odb::dbInst::create(block_, lib_->findMaster("BUF_X1"), "driver");
    driver->findITerm("A")->connect(input);
    auto* output = odb::dbNet::create(block_, "output");
    driver->findITerm("Z")->connect(output);
    pin_ = network_->dbToSta(driver->findITerm("Z"));
    for (const char* name : {"load_a", "load_b"}) {
      auto* load
          = odb::dbInst::create(block_, lib_->findMaster("INV_X1"), name);
      load->findITerm("A")->connect(output);
      load_port_
          = network_->libertyPort(network_->dbToSta(load->findITerm("A")));
    }
  }

  void setDesignFanout(float limit)
  {
    sta_->setFanoutLimit(network_->cell(network_->topInstance()),
                         sta::MinMax::max(),
                         limit,
                         sta_->cmdSdc());
  }

  bool hasHeadroom() { return driverHasHeadroom(sta_.get(), pin_, 0.0, 0.0); }

  sta::dbNetwork* network_ = nullptr;
  sta::Pin* pin_ = nullptr;
  sta::LibertyPort* load_port_ = nullptr;
};

TEST_F(TestDriverHeadroom, MissingFanoutLimitAllowsLoads)
{
  EXPECT_TRUE(hasHeadroom());
}

TEST_F(TestDriverHeadroom, DesignFanoutLimitIncludesBoundaryAndZero)
{
  setDesignFanout(2.0f);
  EXPECT_TRUE(hasHeadroom());
  setDesignFanout(1.0f);
  EXPECT_FALSE(hasHeadroom());
  setDesignFanout(0.0f);
  EXPECT_FALSE(hasHeadroom());
  setDesignFanout(2.0f);
  EXPECT_TRUE(hasHeadroom());
}

TEST_F(TestDriverHeadroom, LibertyPortLimitAppliesWithoutSdcLimit)
{
  network_->libertyPort(pin_)->setFanoutLimit(1.0f, sta::MinMax::max());
  EXPECT_FALSE(hasHeadroom());
  network_->libertyPort(pin_)->setFanoutLimit(2.0f, sta::MinMax::max());
  EXPECT_TRUE(hasHeadroom());
}

TEST_F(TestDriverHeadroom, LibertyDefaultFanoutLimitApplies)
{
  auto* library = network_->libertyPort(pin_)->libertyLibrary();
  library->setDefaultMaxFanout(1.0f);
  EXPECT_FALSE(hasHeadroom());
  library->setDefaultMaxFanout(2.0f);
  EXPECT_TRUE(hasHeadroom());
}

TEST_F(TestDriverHeadroom, TightestDesignAndLibertyLimitApplies)
{
  setDesignFanout(3.0f);
  network_->libertyPort(pin_)->setFanoutLimit(1.0f, sta::MinMax::max());
  EXPECT_FALSE(hasHeadroom());
  setDesignFanout(1.0f);
  network_->libertyPort(pin_)->setFanoutLimit(3.0f, sta::MinMax::max());
  EXPECT_FALSE(hasHeadroom());
  setDesignFanout(2.0f);
  EXPECT_TRUE(hasHeadroom());
}

TEST_F(TestDriverHeadroom, UsesLibertyFanoutLoadInsteadOfSinkCount)
{
  load_port_->setFanoutLoad(1.5f);
  setDesignFanout(2.0f);
  EXPECT_FALSE(hasHeadroom());
  setDesignFanout(3.0f);
  EXPECT_TRUE(hasHeadroom());
  load_port_->setFanoutLoad(0.5f);
  setDesignFanout(1.0f);
  EXPECT_TRUE(hasHeadroom());
}

}  // namespace
}  // namespace wmk
