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

class InstanceBuilder
{
 public:
  InstanceBuilder(utl::Logger* logger,
                  sta::dbNetwork* network,
                  sta::LibertyCell* cell,
                  sta::LibertyPort* clk,
                  std::array<sta::LibertyPort*, 2> inputs,
                  sta::LibertyPort* output,
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

  InstanceBuilder makeAnd(const char* name)
  {
    return InstanceBuilder(
        logger_, network_, and_cell_, nullptr, and_inputs_, and_output_, name);
  }

  InstanceBuilder makeAnd(const std::string& name)
  {
    return makeAnd(name.c_str());
  }
  InstanceBuilder makeOr(const char* name)
  {
    return InstanceBuilder(
        logger_, network_, or_cell_, nullptr, or_inputs_, or_output_, name);
  }

  InstanceBuilder makeOr(const std::string& name)
  {
    return makeOr(name.c_str());
  }
  InstanceBuilder makeNot(const char* name)
  {
    return InstanceBuilder(
        logger_, network_, not_cell_, nullptr, {not_input_}, not_output_, name);
  }

  InstanceBuilder makeNot(const std::string& name)
  {
    return makeNot(name.c_str());
  }
  InstanceBuilder makeClockGate(const char* name)
  {
    return InstanceBuilder(logger_,
                           network_,
                           clkgate_cell_,
                           clkgate_clk_,
                           {clkgate_enable_},
                           clkgate_output_,
                           name);
  }

  InstanceBuilder makeClockGate(const std::string& name)
  {
    return makeClockGate(name.c_str());
  }

 private:
  utl::Logger* logger_ = nullptr;
  sta::dbNetwork* network_ = nullptr;

  sta::LibertyCell* and_cell_ = nullptr;
  std::array<sta::LibertyPort*, 2> and_inputs_;
  sta::LibertyPort* and_output_ = nullptr;

  sta::LibertyCell* or_cell_ = nullptr;
  std::array<sta::LibertyPort*, 2> or_inputs_;
  sta::LibertyPort* or_output_ = nullptr;

  sta::LibertyCell* not_cell_ = nullptr;
  sta::LibertyPort* not_input_ = nullptr;
  sta::LibertyPort* not_output_ = nullptr;

  sta::LibertyCell* clkgate_cell_ = nullptr;
  sta::LibertyPort* clkgate_clk_ = nullptr;
  sta::LibertyPort* clkgate_enable_ = nullptr;
  sta::LibertyPort* clkgate_output_ = nullptr;
};

inline InstanceBuilder::InstanceBuilder(utl::Logger* logger,
                                        sta::dbNetwork* network,
                                        sta::LibertyCell* cell,
                                        sta::LibertyPort* clk,
                                        std::array<sta::LibertyPort*, 2> inputs,
                                        sta::LibertyPort* output,
                                        const char* name)
    : logger_(logger),
      network_(network),
      clk_(clk),
      inputs_(inputs),
      output_(output)
{
  inst_ = network_->makeInstance(cell, name, network_->topInstance());
}

inline sta::Net* InstanceBuilder::makeOutputNet(const char* name)
{
  auto net = network_->makeNet(name, network_->topInstance());
  connectOutput(net);
  return net;
}

}  // namespace cgt
