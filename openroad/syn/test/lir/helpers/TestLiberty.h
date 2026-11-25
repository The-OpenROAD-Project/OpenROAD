// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// A small Liberty library for L-IR tests, with helpers to build its cells
// by hand.

#pragma once

#include <initializer_list>
#include <string_view>
#include <vector>

#include "liberty/LibertyBuilder.hh"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"

namespace syn::test {

// Owns a Liberty library. Declare it before any Design that interns its
// cells, so that it outlives the Design.
class TestLiberty
{
 public:
  explicit TestLiberty(std::string_view name = "test",
                       std::string_view filename = "test.lib");
  TestLiberty(const TestLiberty&) = delete;
  TestLiberty& operator=(const TestLiberty&) = delete;

  sta::LibertyLibrary& library() { return lib_; }
  sta::LibertyBuilder& builder() { return builder_; }

  // A cell without ports.
  sta::LibertyCell* cell(std::string_view name);
  sta::LibertyPort* port(sta::LibertyCell* cell,
                         std::string_view name,
                         const sta::PortDirection* direction);
  // A cell with these input ports, then these output ports; every output's
  // function is the AND of the inputs (0 without inputs). Replace one with
  // setFunction().
  sta::LibertyCell* combinational(
      std::string_view name,
      std::initializer_list<std::string_view> inputs,
      std::initializer_list<std::string_view> outputs);
  // The half adder HA(A, B) -> (S = A ^ B, CO = A & B).
  sta::LibertyCell* halfAdder(std::string_view name = "HA");
  // A flip-flop DFF(D, <clock>) -> (Q, QN), whose state is Q and QN.
  sta::LibertyCell* dff(std::string_view name = "DFF",
                        std::string_view clock = "CLK");

  // DFFRS(D, CK, RN, SN) -> (Q = IQ, QN = IQN): active-low clear RN and
  // preset SN, with these clear_preset_var1 and clear_preset_var2; `outputs`
  // may leave Q or QN out.
  sta::LibertyCell* dffrs(std::string_view name,
                          sta::LogicValue var1,
                          sta::LogicValue var2,
                          std::initializer_list<std::string_view> outputs
                          = {"Q", "QN"});

  // Makes `cell` sequential (a flip-flop, or a latch if !is_register), with
  // state ports `q` and `qn` (may be null), and no clock or data function.
  static void flop(sta::LibertyCell* cell,
                   bool is_register,
                   sta::LibertyPort* q,
                   sta::LibertyPort* qn);
  // Sets the function of scalar port `port`, freeing the one it had
  // (LibertyPort::setFunction does not).
  static void setFunction(sta::LibertyPort* port, sta::FuncExpr* function);
  // The AND of `ports` (constant 0 if empty).
  static sta::FuncExpr* andOf(const std::vector<sta::LibertyPort*>& ports);
  // Gives `cell` an empty statetable over these ports.
  static void statetable(sta::LibertyCell* cell,
                         sta::LibertyPortSeq inputs,
                         sta::LibertyPortSeq internals);

 private:
  sta::LibertyLibrary lib_;
  sta::LibertyBuilder builder_{nullptr, nullptr};
};

}  // namespace syn::test
