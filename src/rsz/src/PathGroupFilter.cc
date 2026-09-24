// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "PathGroupFilter.hh"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/ExceptionPath.hh"
#include "sta/Graph.hh"
#include "sta/Mode.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PathEnd.hh"
#include "sta/PathGroup.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/SdcClass.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace rsz {

namespace {

// The "<start>2<end>" group names split back into the two ends they describe.
// gated_clock is the exception: OpenSTA groups every clock gating check
// together regardless of where the enable path started.
StartpointKind groupStartpointKind(const PathGroupType type)
{
  switch (type) {
    case PathGroupType::kIn2Reg:
    case PathGroupType::kIn2Out:
      return StartpointKind::kPrimaryInput;
    case PathGroupType::kReg2Reg:
    case PathGroupType::kReg2Out:
      return StartpointKind::kRegister;
    case PathGroupType::kGatedClock:
    case PathGroupType::kNone:
      break;
  }
  return StartpointKind::kAny;
}

EndpointKind groupEndpointKind(const PathGroupType type)
{
  switch (type) {
    case PathGroupType::kReg2Out:
    case PathGroupType::kIn2Out:
      return EndpointKind::kPrimaryOutput;
    case PathGroupType::kGatedClock:
      return EndpointKind::kGatedClockEnable;
    case PathGroupType::kReg2Reg:
    case PathGroupType::kIn2Reg:
    case PathGroupType::kNone:
      break;
  }
  return EndpointKind::kRegister;
}

bool groupStartsAtInput(const PathGroupType type)
{
  return groupStartpointKind(type) == StartpointKind::kPrimaryInput;
}

// Labels for the "path_group" debug channel.
const char* endpointKindName(const EndpointKind kind)
{
  switch (kind) {
    case EndpointKind::kRegister:
      return "register";
    case EndpointKind::kPrimaryOutput:
      return "primary_output";
    case EndpointKind::kGatedClockEnable:
      return "gated_clock_enable";
  }
  return "?";
}

// The name OpenSTA knows the group by.  Only gated_clock differs: OpenSTA
// collects every clock gating check under a built-in group of its own, so
// there is never a group_path to make for it.
std::string_view staPathGroupName(const PathGroupType type)
{
  switch (type) {
    case PathGroupType::kReg2Reg:
      return "reg2reg";
    case PathGroupType::kIn2Reg:
      return "in2reg";
    case PathGroupType::kReg2Out:
      return "reg2out";
    case PathGroupType::kIn2Out:
      return "in2out";
    case PathGroupType::kGatedClock:
      return sta::PathGroups::gatedClkGroupName();
    case PathGroupType::kNone:
      break;
  }
  return "";
}

// Pins of the top level ports on one side of the design.  Mirrors
// all_inputs -no_clocks / all_outputs.  Caller owns the set.
sta::PinSet* primaryPins(Resizer* resizer, const bool inputs)
{
  sta::dbSta* sta = resizer->sta();
  sta::Network* network = resizer->network();
  auto* pins = new sta::PinSet(network);
  std::unique_ptr<sta::InstancePinIterator> pin_iter(
      network->pinIterator(network->topInstance()));
  while (pin_iter->hasNext()) {
    const sta::Pin* pin = pin_iter->next();
    const sta::PortDirection* direction = network->direction(pin);
    if (inputs) {
      if (direction->isAnyInput() && !sta->isClockSrc(pin, sta->cmdSdc())) {
        pins->insert(pin);
      }
    } else if (direction->isAnyOutput()) {
      pins->insert(pin);
    }
  }
  return pins;
}

// Register/latch data or clock pins.  Mirrors
// all_registers -data_pins / -clock_pins.  Caller owns the set.
sta::PinSet* registerPins(Resizer* resizer, const bool data_pins)
{
  sta::dbSta* sta = resizer->sta();
  sta::Mode* mode = sta->cmdMode();
  sta::PinSet pins
      = data_pins ? sta->findRegisterDataPins(/*clks=*/nullptr,
                                              sta::RiseFallBoth::riseFall(),
                                              /*registers=*/true,
                                              /*latches=*/true,
                                              mode)
                  : sta->findRegisterClkPins(/*clks=*/nullptr,
                                             sta::RiseFallBoth::riseFall(),
                                             /*registers=*/true,
                                             /*latches=*/true,
                                             mode);
  return new sta::PinSet(std::move(pins));
}

// Makes the group_path that matches `type`, unless the design has nothing on
// one of its two sides.
void makePathGroup(Resizer* resizer,
                   const std::string& name,
                   const PathGroupType type)
{
  const EndpointKind endpoint_kind = groupEndpointKind(type);
  if (endpoint_kind == EndpointKind::kGatedClockEnable) {
    // Clock gating check ends are found by OpenSTA rather than named by pin,
    // so there is no group_path that expresses them.  OpenSTA already reports
    // them under its own built-in group, so nothing has to be made here.
    return;
  }

  sta::dbSta* sta = resizer->sta();
  sta::Sdc* sdc = sta->cmdSdc();
  sta::PinSet* from_pins = groupStartsAtInput(type)
                               ? primaryPins(resizer, /*inputs=*/true)
                               : registerPins(resizer, /*data_pins=*/false);
  sta::PinSet* to_pins = endpoint_kind == EndpointKind::kPrimaryOutput
                             ? primaryPins(resizer, /*inputs=*/false)
                             : registerPins(resizer, /*data_pins=*/true);
  if (from_pins->empty() || to_pins->empty()) {
    // No path in the design can belong to the group.  repair_timing still
    // honors the restriction; it simply finds nothing to repair.
    delete from_pins;
    delete to_pins;
    return;
  }

  sta::ExceptionFrom* from
      = sta->makeExceptionFrom(from_pins,
                               /*from_clks=*/nullptr,
                               /*from_insts=*/nullptr,
                               sta::RiseFallBoth::riseFall(),
                               sdc);
  sta::ExceptionTo* to = sta->makeExceptionTo(to_pins,
                                              /*to_clks=*/nullptr,
                                              /*to_insts=*/nullptr,
                                              sta::RiseFallBoth::riseFall(),
                                              sta::RiseFallBoth::riseFall(),
                                              sdc);
  sta->makeGroupPath(
      name, /*is_default=*/false, from, /*thrus=*/nullptr, to, "", sdc);
  resizer->logger()->info(utl::RSZ, 226, "Created path group '{}'.", name);
}

}  // namespace

const std::vector<std::string_view>& pathGroupNames()
{
  static const std::vector<std::string_view> names{
      "reg2reg", "in2reg", "reg2out", "in2out", "gated_clock"};
  return names;
}

PathGroupType findPathGroupType(const std::string_view name)
{
  if (name == "reg2reg") {
    return PathGroupType::kReg2Reg;
  }
  if (name == "in2reg") {
    return PathGroupType::kIn2Reg;
  }
  if (name == "reg2out") {
    return PathGroupType::kReg2Out;
  }
  if (name == "in2out") {
    return PathGroupType::kIn2Out;
  }
  if (name == "gated_clock") {
    return PathGroupType::kGatedClock;
  }
  return PathGroupType::kNone;
}

std::string resolvePathGroupName(Resizer* resizer, const char* name)
{
  const std::string group_name = name != nullptr ? name : "";
  const PathGroupType type = findPathGroupType(group_name);
  if (type == PathGroupType::kNone) {
    std::string valid;
    for (const std::string_view path_group_name : pathGroupNames()) {
      if (!valid.empty()) {
        valid += ", ";
      }
      valid += path_group_name;
    }
    resizer->logger()->warn(utl::RSZ,
                            225,
                            "Unknown -path_group '{}'. Valid path groups are "
                            "{}. Timing optimization is not restricted to a "
                            "path group.",
                            group_name,
                            valid);
    return "";
  }

  sta::dbSta* sta = resizer->sta();
  if (!sta->isPathGroupName(staPathGroupName(type), sta->cmdSdc())) {
    makePathGroup(resizer, group_name, type);
  }
  return group_name;
}

PathGroupFilter::PathGroupFilter(Resizer* resizer)
    : sta_(resizer->sta()),
      network_(resizer->network()),
      sdc_(resizer->sta()->cmdSdc()),
      logger_(resizer->logger()),
      type_(findPathGroupType(resizer->pathGroup()))
{
  if (enabled()) {
    // Endpoint classification asks whether pins are on the clock network.
    // No-op once the network is up to date.
    sta_->ensureClkNetwork(sta_->cmdMode());
  }
}

bool PathGroupFilter::isPrimaryInput(const sta::Pin* pin) const
{
  return network_->isTopLevelPort(pin)
         && network_->direction(pin)->isAnyInput();
}

bool PathGroupFilter::isPrimaryOutput(const sta::Pin* pin) const
{
  return network_->isTopLevelPort(pin)
         && network_->direction(pin)->isAnyOutput();
}

// A clock gating check ends at the enable input of a gate whose output drives
// the clock network.  OpenSTA reports those paths in its own "gated clock"
// group, so they must not be counted as register endpoints.
bool PathGroupFilter::isGatedClockEnable(const sta::Vertex* vertex) const
{
  const sta::Pin* pin = vertex->pin();
  const sta::Mode* mode = sta_->cmdMode();
  if (sta_->isClock(pin, mode)) {
    // On the clock network itself, so this is the gate's clock side, not the
    // enable side.
    return false;
  }
  const sta::Instance* inst = network_->instance(pin);
  if (inst == nullptr || inst == network_->topInstance()) {
    return false;
  }
  std::unique_ptr<sta::InstancePinIterator> pin_iter(
      network_->pinIterator(inst));
  while (pin_iter->hasNext()) {
    const sta::Pin* inst_pin = pin_iter->next();
    if (network_->direction(inst_pin)->isAnyOutput()
        && sta_->isClock(inst_pin, mode)) {
      return true;
    }
  }
  return false;
}

EndpointKind PathGroupFilter::endpointKind(const sta::Vertex* endpoint) const
{
  const sta::Pin* pin = endpoint->pin();
  if (isPrimaryOutput(pin)) {
    return EndpointKind::kPrimaryOutput;
  }
  if (isGatedClockEnable(endpoint)) {
    return EndpointKind::kGatedClockEnable;
  }
  return EndpointKind::kRegister;
}

bool PathGroupFilter::endpointInGroup(sta::Vertex* endpoint,
                                      const sta::MinMax* min_max) const
{
  if (!enabled()) {
    return true;
  }
  if (endpoint == nullptr) {
    return false;
  }
  // Endpoint side is structural: a primary output ends *2out paths, a clock
  // gate enable ends gated_clock paths, a register/latch data pin ends *2reg
  // paths.
  const EndpointKind kind = endpointKind(endpoint);
  if (kind != groupEndpointKind(type_)) {
    debugPrint(logger_,
               utl::RSZ,
               "path_group",
               2,
               "{} rejected from {}: endpoint is {}",
               network_->pathName(endpoint->pin()),
               staPathGroupName(type_),
               endpointKindName(kind));
    return false;
  }
  // The endpoint kind above is exact and cheap, but it cannot tell reg2reg
  // from in2reg (nor reg2out from in2out) because those groups share their
  // endpoints and differ only in where the path started.  Ask OpenSTA for the
  // group's own slack at this endpoint rather than guessing from the
  // endpoint's overall worst path, which frequently belongs to another group.
  const std::optional<sta::Slack> slack = groupSlack(endpoint, min_max);
  if (!slack.has_value()) {
    debugPrint(logger_,
               utl::RSZ,
               "path_group",
               2,
               "{} rejected from {}: no path in group",
               network_->pathName(endpoint->pin()),
               staPathGroupName(type_));
    return false;
  }
  debugPrint(logger_,
             utl::RSZ,
             "path_group",
             2,
             "{} kept in {}: group slack {} (endpoint slack {})",
             network_->pathName(endpoint->pin()),
             staPathGroupName(type_),
             sta::delayAsString(*slack, 4, sta_),
             sta::delayAsString(sta_->slack(endpoint, min_max), 4, sta_));
  return true;
}

std::optional<sta::Slack> PathGroupFilter::groupSlack(
    sta::Vertex* endpoint,
    const sta::MinMax* min_max) const
{
  if (!enabled()) {
    return sta_->slack(endpoint, min_max);
  }
  if (endpoint == nullptr) {
    return std::nullopt;
  }

  // findPathEnds takes ownership of the ExceptionTo.
  auto* to_pins = new sta::PinSet(network_);
  to_pins->insert(endpoint->pin());
  sta::ExceptionTo* to = sta_->makeExceptionTo(to_pins,
                                               /*to_clks=*/nullptr,
                                               /*to_insts=*/nullptr,
                                               sta::RiseFallBoth::riseFall(),
                                               sta::RiseFallBoth::riseFall(),
                                               sdc_);

  const bool setup = min_max == sta::MinMax::max();
  sta::StringSeq group_names{std::string(staPathGroupName(type_))};
  const sta::PathEndSeq ends = sta_->findPathEnds(
      /*from=*/nullptr,
      /*thrus=*/nullptr,
      to,
      /*unconstrained=*/false,
      sta_->scenes(),
      setup ? sta::MinMaxAll::max() : sta::MinMaxAll::min(),
      /*group_path_count=*/1,
      /*endpoint_path_count=*/1,
      /*unique_pins=*/true,
      /*unique_edges=*/false,
      -sta::INF,
      sta::INF,
      /*sort_by_slack=*/true,
      group_names,
      /*setup=*/setup,
      /*hold=*/!setup,
      /*recovery=*/false,
      /*removal=*/false,
      /*clk_gating_setup=*/setup,
      /*clk_gating_hold=*/!setup);
  if (ends.empty()) {
    return std::nullopt;
  }
  sta::Slack slack = ends[0]->slack(sta_);
  for (sta::PathEnd* end : ends) {
    delete end;
  }
  return slack;
}

bool PathGroupFilter::startpointInGroup(sta::Vertex* startpoint) const
{
  if (!enabled()) {
    return true;
  }
  if (startpoint == nullptr) {
    return false;
  }
  if (groupStartpointKind(type_) == StartpointKind::kAny) {
    return true;
  }
  return isPrimaryInput(startpoint->pin()) == groupStartsAtInput(type_);
}

}  // namespace rsz
