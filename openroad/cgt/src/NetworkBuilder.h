// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include <array>
#include <cstddef>
#include <string>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/Liberty.hh"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace cgt {

// A library cell and the ports of it that InstanceBuilder connects.  Unused
// ports are null.
struct CellPorts
{
  sta::LibertyCell* cell = nullptr;
  sta::LibertyPort* clk = nullptr;
  std::array<sta::LibertyPort*, 2> inputs{};
  sta::LibertyPort* output = nullptr;
};

class InstanceBuilder
{
 public:
  InstanceBuilder(utl::Logger* logger,
                  sta::dbNetwork* network,
                  const CellPorts& ports,
                  const char* name);

  InstanceBuilder& connectClock(sta::Net* net)
  {
    connectPort(clk_, net);
    return *this;
  }
  template <size_t I>
  InstanceBuilder& connectInput(sta::Net* net)
  {
    static_assert(I < std::tuple_size<decltype(inputs_)>::value,
                  "Input index out of bounds");
    connectPort(inputs_[I], net);
    return *this;
  }
  InstanceBuilder& connectOutput(sta::Net* net)
  {
    connectPort(output_, net);
    return *this;
  }
  sta::Net* makeOutputNet(const char* name);
  sta::Net* makeOutputNet(const std::string& name)
  {
    return makeOutputNet(name.c_str());
  }

 private:
  InstanceBuilder& connectPort(sta::LibertyPort* port, sta::Net* net);

  utl::Logger* logger_;
  sta::dbNetwork* network_;
  sta::LibertyPort* clk_ = nullptr;
  std::array<sta::LibertyPort*, 2> inputs_;
  sta::LibertyPort* output_ = nullptr;
  sta::Instance* inst_;
};

class NetworkBuilder
{
 public:
  void init(utl::Logger* logger, sta::dbNetwork* network);

  InstanceBuilder makeAnd(const std::string& name)
  {
    return InstanceBuilder(logger_, network_, and_, name.c_str());
  }
  InstanceBuilder makeOr(const std::string& name)
  {
    return InstanceBuilder(logger_, network_, or_, name.c_str());
  }
  InstanceBuilder makeNot(const std::string& name)
  {
    return InstanceBuilder(logger_, network_, not_, name.c_str());
  }
  InstanceBuilder makeClockGate(const std::string& name)
  {
    return InstanceBuilder(logger_, network_, clkgate_, name.c_str());
  }

 private:
  // Uses cell as the clock gate if it has a posedge latch, and warns
  // otherwise.
  void setClockGate(sta::LibertyCell* cell);

  utl::Logger* logger_ = nullptr;
  sta::dbNetwork* network_ = nullptr;

  // Two-input AND and OR, inverter, and clock gate cells
  CellPorts and_;
  CellPorts or_;
  CellPorts not_;
  CellPorts clkgate_;
};

inline InstanceBuilder::InstanceBuilder(utl::Logger* logger,
                                        sta::dbNetwork* network,
                                        const CellPorts& ports,
                                        const char* name)
    : logger_(logger),
      network_(network),
      clk_(ports.clk),
      inputs_(ports.inputs),
      output_(ports.output)
{
  inst_ = network_->makeInstance(ports.cell, name, network_->topInstance());
}

inline sta::Net* InstanceBuilder::makeOutputNet(const char* name)
{
  auto net = network_->makeNet(name, network_->topInstance());
  connectOutput(net);
  return net;
}

}  // namespace cgt
