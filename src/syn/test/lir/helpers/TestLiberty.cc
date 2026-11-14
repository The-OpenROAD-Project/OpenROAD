// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "helpers/TestLiberty.h"

#include <initializer_list>
#include <string_view>
#include <vector>

#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
// Defines sta::StatetableRow, which LibertyCell's statetable vector needs.
#include "sta/Sequential.hh"  // IWYU pragma: keep

namespace syn::test {

TestLiberty::TestLiberty(std::string_view name, std::string_view filename)
    : lib_(name, filename)
{
}

sta::LibertyCell* TestLiberty::cell(std::string_view name)
{
  return builder_.makeCell(&lib_, name, lib_.filename());
}

sta::LibertyPort* TestLiberty::port(sta::LibertyCell* cell,
                                    std::string_view name,
                                    sta::PortDirection* direction)
{
  sta::LibertyPort* port = builder_.makePort(cell, name);
  port->setDirection(direction);
  return port;
}

sta::LibertyCell* TestLiberty::combinational(
    std::string_view name,
    std::initializer_list<std::string_view> inputs,
    std::initializer_list<std::string_view> outputs)
{
  sta::LibertyCell* c = cell(name);
  std::vector<sta::LibertyPort*> ins;
  for (const std::string_view input : inputs) {
    ins.push_back(port(c, input, sta::PortDirection::input()));
  }
  for (const std::string_view output : outputs) {
    port(c, output, sta::PortDirection::output())->setFunction(andOf(ins));
  }
  return c;
}

sta::LibertyCell* TestLiberty::halfAdder(std::string_view name)
{
  sta::LibertyCell* c = cell(name);
  sta::LibertyPort* a = port(c, "A", sta::PortDirection::input());
  sta::LibertyPort* b = port(c, "B", sta::PortDirection::input());
  // setFunction() does not free a previous function: set each one once.
  port(c, "S", sta::PortDirection::output())
      ->setFunction(sta::FuncExpr::makeXor(sta::FuncExpr::makePort(a),
                                           sta::FuncExpr::makePort(b)));
  port(c, "CO", sta::PortDirection::output())->setFunction(andOf({a, b}));
  return c;
}

sta::LibertyCell* TestLiberty::dff(std::string_view name,
                                   std::string_view clock)
{
  sta::LibertyCell* c = combinational(name, {"D", clock}, {});
  sta::LibertyPort* q = port(c, "Q", sta::PortDirection::output());
  sta::LibertyPort* qn = port(c, "QN", sta::PortDirection::output());
  flop(c, true, q, qn);
  return c;
}

sta::FuncExpr* TestLiberty::andOf(const std::vector<sta::LibertyPort*>& ports)
{
  sta::FuncExpr* expr = nullptr;
  for (sta::LibertyPort* p : ports) {
    sta::FuncExpr* term = sta::FuncExpr::makePort(p);
    expr = expr == nullptr ? term : sta::FuncExpr::makeAnd(expr, term);
  }
  return expr == nullptr ? sta::FuncExpr::makeZero() : expr;
}

void TestLiberty::flop(sta::LibertyCell* cell,
                       bool is_register,
                       sta::LibertyPort* q,
                       sta::LibertyPort* qn)
{
  cell->makeSequential(1,
                       is_register,
                       nullptr,
                       nullptr,
                       nullptr,
                       nullptr,
                       sta::LogicValue::unknown,
                       sta::LogicValue::unknown,
                       q,
                       qn);
}

void TestLiberty::statetable(sta::LibertyCell* cell,
                             sta::LibertyPortSeq inputs,
                             sta::LibertyPortSeq internals)
{
  sta::StatetableRows rows;
  cell->makeStatetable(inputs, internals, rows);
}

}  // namespace syn::test
