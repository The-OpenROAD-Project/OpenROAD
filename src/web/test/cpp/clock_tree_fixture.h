// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "sta/NetworkClass.hh"
#include "sta/SdcClass.hh"
#include "sta/Units.hh"
#include "tst/db_fixture.h"
#include "tst/nangate45_fixture.h"
#include "web/clockTree.h"
#include "web/descriptor_registry.h"

namespace web {

// clk drives b0, which splits to b1 and b2; b1 splits again to b3 and b4.
// b3, b4 and b2 clock the registers.
class ClockTreeTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    readLiberty("_main/test/Nangate45/Nangate45_typ.lib");
    block_->setDieArea(odb::Rect(0, 0, 100000, 100000));
    DescriptorRegistry::instance()->initDescriptors(getDb(), getSta());

    odb::dbMaster* buf = lib_->findMaster("BUF_X1");
    odb::dbMaster* dff = lib_->findMaster("DFF_X1");
    ASSERT_NE(buf, nullptr);
    ASSERT_NE(dff, nullptr);
    place(buf, "b0", {10000, 50000}, {{"clk", "A"}, {"c0", "Z"}});
    place(buf, "b1", {30000, 75000}, {{"c0", "A"}, {"c1", "Z"}});
    place(buf, "b2", {30000, 25000}, {{"c0", "A"}, {"c2", "Z"}});
    place(buf, "b3", {55000, 90000}, {{"c1", "A"}, {"c3", "Z"}});
    place(buf, "b4", {55000, 60000}, {{"c1", "A"}, {"c4", "Z"}});
    place(dff, "ff0", {85000, 90000}, {{"c3", "CK"}});
    place(dff, "ff1", {85000, 60000}, {{"c4", "CK"}});
    place(dff, "ff2", {85000, 35000}, {{"c2", "CK"}});
    place(dff, "ff3", {85000, 10000}, {{"c2", "CK"}});
    odb::dbBTerm* clk = odb::dbBTerm::create(block_->findNet("clk"), "clk");
    clk->setIoType(odb::dbIoType::INPUT);

    sta_->postReadDef(block_);
    sta_->getDbNetwork()->setBlock(block_);
    defineClock();
  }

  // create_clock on the clk port, as a read_sdc would; defining it again
  // replaces the clock.
  void defineClock()
  {
    sta::dbNetwork* network = sta_->getDbNetwork();
    sta::PinSet pins(network);
    pins.insert(network->dbToSta(block_->findBTerm("clk")));
    const float period = sta_->units()->timeUnit()->userToSta(1.0);
    sta_->makeClock("clk",
                    pins,
                    false,
                    period,
                    sta::FloatSeq{0.0, period / 2},
                    "",
                    sta_->cmdMode());
  }

  // Each pair is {net, term}, as in InstOptions::ITermInfo.
  void place(odb::dbMaster* master,
             const char* name,
             const odb::Point& location,
             const std::vector<std::pair<const char*, const char*>>& pins)
  {
    tst::InstOptions opts;
    opts.location = location;
    opts.status = odb::dbPlacementStatus::PLACED;
    for (const auto& [net, term] : pins) {
      opts.iterms.push_back({.net_name = net, .term_name = term});
    }
    makeInst(block_, master, name, opts);
  }

  std::vector<std::unique_ptr<ClockTree>> trees()
  {
    return buildClockTrees(sta_.get(), sta_->scenes().front());
  }
};

}  // namespace web
