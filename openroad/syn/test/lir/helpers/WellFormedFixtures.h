// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Fixtures shared by the WellFormed tests: well_formed_test (built modules)
// and well_formed_edits_test (modules broken by Editor edits).

#pragma once

#include <functional>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "gtest/gtest.h"
#include "helpers/TestDesignOptions.h"
#include "helpers/TestLiberty.h"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/PortDirection.hh"
#include "syn/lir/Design.h"
#include "syn/lir/Ids.h"
#include "syn/lir/LibCellTable.h"
#include "syn/lir/Module.h"
#include "syn/lir/ModuleBuilder.h"
#include "syn/lir/RegClassPool.h"
#include "syn/lir/RegPinTypes.h"
#include "syn/lir/WellFormed.h"

namespace syn::test {

// A fresh port name, interned in `design`: every call returns a new one.
inline lir::NameId pn(lir::Design& design)
{
  static int next = 0;
  return design.names().intern(absl::StrCat("port", next++));
}

// Every test builds in write mode; the check works in both modes.
class WellFormedTest : public ::testing::Test
{
 protected:
  absl::Status check(lir::ModuleBuilder&& builder)
  {
    absl::StatusOr<lir::Module> module = std::move(builder).finish();
    EXPECT_TRUE(module.ok()) << module.status();
    if (!module.ok()) {
      return module.status();
    }
    return lir::checkWellFormed(design_, *module);
  }

  std::string error(lir::ModuleBuilder&& builder)
  {
    const absl::Status status = check(std::move(builder));
    EXPECT_FALSE(status.ok());
    return std::string(status.message());
  }

  static lir::RegClass regClass(std::initializer_list<lir::RegPinType> pins,
                                lir::RegFlags flags = lir::RegFlags(),
                                lir::RegInit init = {})
  {
    lir::RegPinMask mask = 0;
    for (const lir::RegPinType pin : pins) {
      mask |= lir::regPinBit(pin);
    }
    return lir::RegClass::generic(flags, mask, init);
  }

  // A register of `width` bits whose RegIn pins come from `make_pins`.
  lir::ModuleBuilder registerModule(
      int width,
      const lir::RegClass& reg_class,
      const std::function<std::vector<lir::ObjId>(lir::ModuleBuilder&,
                                                  lir::ObjId)>& make_pins)
  {
    lir::ModuleBuilder b(design_.names().intern("reg"));
    return registerModule(
        b, width, b.regClasses().intern(reg_class), make_pins);
  }
  lir::ModuleBuilder registerModule(
      int width,
      lir::RegClassId reg_class,
      const std::function<std::vector<lir::ObjId>(lir::ModuleBuilder&,
                                                  lir::ObjId)>& make_pins)
  {
    lir::ModuleBuilder b(design_.names().intern("reg"));
    return registerModule(b, width, reg_class, make_pins);
  }
  lir::ModuleBuilder registerModule(
      lir::ModuleBuilder& b,
      int width,
      lir::RegClassId reg_class,
      const std::function<std::vector<lir::ObjId>(lir::ModuleBuilder&,
                                                  lir::ObjId)>& make_pins)
  {
    const lir::ObjId q = b.addRegisterOut(width, reg_class);
    b.addRegisterIn(q, make_pins(b, q));
    b.addPo(pn(design_), q);
    return std::move(b);
  }

  TestLiberty lib_;  // outlives design_, which points to its cells
  lir::Design design_{kCheckWellFormed};
  lir::Design::SingleWriteScope write_{design_};
};

class WellFormedCellTest : public WellFormedTest
{
 protected:
  void SetUp() override
  {
    // HA(A, B) -> (S, CO) and DFF(D, CLK) -> (Q, QN).
    ha_id_ = design_.cells().intern(lib_.halfAdder());
    dff_id_ = design_.cells().intern(lib_.dff());
    const sta::PortDirection* in = sta::PortDirection::input();
    const sta::PortDirection* internal = sta::PortDirection::internal();

    // Clock gate: ICG(CLK, EN) -> GCLK, with a statetable as in Liberty.
    sta::LibertyCell* icg_cell = lib_.cell("ICG");
    sta::LibertyPort* icg_clk = lib_.port(icg_cell, "CLK", in);
    sta::LibertyPort* icg_en = lib_.port(icg_cell, "EN", in);
    lib_.port(icg_cell, "GCLK", sta::PortDirection::output());
    sta::LibertyPort* icg_iq = lib_.port(icg_cell, "IQ", internal);
    TestLiberty::statetable(icg_cell, {icg_clk, icg_en}, {icg_iq});
    icg_cell->setClockGateType(sta::ClockGateType::latch_posedge);
    icg_id_ = design_.cells().intern(icg_cell);

    // Flop with a tristate output: TDFF(D, CLK, OE) -> Q, a black box.
    sta::LibertyCell* tdff_cell = lib_.cell("TDFF");
    lib_.port(tdff_cell, "D", in);
    lib_.port(tdff_cell, "CLK", in);
    sta::LibertyPort* tdff_oe = lib_.port(tdff_cell, "OE", in);
    sta::LibertyPort* tdff_q
        = lib_.port(tdff_cell, "Q", sta::PortDirection::tristate());
    sta::LibertyPort* tdff_iq = lib_.port(tdff_cell, "IQ", internal);
    TestLiberty::flop(tdff_cell, true, tdff_iq, nullptr);
    tdff_q->setFunction(sta::FuncExpr::makePort(tdff_iq));
    tdff_q->setTristateEnable(sta::FuncExpr::makePort(tdff_oe));
    tdff_id_ = design_.cells().intern(tdff_cell);
    ASSERT_EQ(design_.cells().info(icg_id_).kind(),
              lir::LibCellKind::kClockGate);
    ASSERT_EQ(design_.cells().info(tdff_id_).kind(),
              lir::LibCellKind::kBlackBox);
  }

  lir::LibCellId ha_id_;
  lir::LibCellId dff_id_;
  lir::LibCellId icg_id_;
  lir::LibCellId tdff_id_;
};

}  // namespace syn::test
