// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SizeUpMatchGenerator.hh"

#include <memory>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "SizeUpMatchCandidate.hh"
#include "rsz/Resizer.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

SizeUpMatchGenerator::SizeUpMatchGenerator(const GeneratorContext& context)
    : MoveGenerator(context)
{
}

bool SizeUpMatchGenerator::isApplicable(const Target& target) const
{
  return MoveGenerator::isApplicable(target) && target.path_index >= 2;
}

std::vector<std::unique_ptr<MoveCandidate>> SizeUpMatchGenerator::generate(
    const Target& target)
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  sta::Pin* drvr_pin = nullptr;
  sta::Instance* inst = nullptr;
  sta::LibertyCell* curr_cell = nullptr;
  if (!resolveDriverTarget(target, drvr_pin, inst, curr_cell)) {
    return candidates;
  }

  sta::Pin* prev_drvr_pin = nullptr;
  if (!loadPreviousDriverPin(target, prev_drvr_pin)) {
    debugPrint(resizer_.logger(),
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: No previous driver pin",
               resizer_.network()->pathName(drvr_pin));
    return candidates;
  }
  int prev_drvr_fanout = 0;
  if (!hasSingleStageFanout(prev_drvr_pin, prev_drvr_fanout)) {
    if (prev_drvr_fanout > 1) {
      debugPrint(resizer_.logger(),
                 RSZ,
                 "size_up_match_move",
                 2,
                 "REJECT SizeUpMatchMove {}: Previous driver fanout {} > 1",
                 resizer_.network()->pathName(drvr_pin),
                 prev_drvr_fanout);
    } else {
      debugPrint(resizer_.logger(),
                 RSZ,
                 "size_up_match_move",
                 2,
                 "REJECT SizeUpMatchMove {}: No previous driver vertex",
                 resizer_.network()->pathName(drvr_pin));
    }
    return candidates;
  }

  sta::LibertyCell* replacement = selectReplacement(curr_cell, prev_drvr_pin);
  if (replacement == nullptr) {
    return candidates;
  }

  candidates.push_back(std::make_unique<SizeUpMatchCandidate>(
      resizer_, target, drvr_pin, inst, replacement));
  return candidates;
}

bool SizeUpMatchGenerator::resolveDriverTarget(
    const Target& target,
    sta::Pin*& drvr_pin,
    sta::Instance*& inst,
    sta::LibertyCell*& curr_cell) const
{
  drvr_pin = target.driver_pin;
  if (drvr_pin == nullptr) {
    drvr_pin = target.endpoint_path->pin(resizer_.staState());
  }
  if (drvr_pin == nullptr) {
    return false;
  }

  inst = resizer_.network()->instance(drvr_pin);
  if (inst == nullptr) {
    debugPrint(resizer_.logger(),
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: No driver instance",
               resizer_.network()->pathName(drvr_pin));
    return false;
  }
  if (resizer_.dontTouch(inst)) {
    debugPrint(resizer_.logger(),
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: {} is \"don't touch\"",
               resizer_.network()->pathName(drvr_pin),
               resizer_.network()->pathName(inst));
    return false;
  }
  if (!resizer_.isLogicStdCell(inst)) {
    debugPrint(resizer_.logger(),
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: {} isn't logic std cell",
               resizer_.network()->pathName(drvr_pin),
               resizer_.network()->pathName(inst));
    return false;
  }

  sta::LibertyPort* drvr_port = resizer_.network()->libertyPort(drvr_pin);
  curr_cell = drvr_port != nullptr ? drvr_port->libertyCell() : nullptr;
  if (curr_cell == nullptr) {
    debugPrint(resizer_.logger(),
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: No liberty cell found for {}",
               resizer_.network()->pathName(drvr_pin),
               resizer_.network()->pathName(inst));
    return false;
  }
  return true;
}

bool SizeUpMatchGenerator::loadPreviousDriverPin(const Target& target,
                                                 sta::Pin*& prev_drvr_pin) const
{
  const sta::Path* prev_drvr_path = target.prevDriverPath(resizer_);
  prev_drvr_pin = prev_drvr_path != nullptr
                      ? prev_drvr_path->pin(resizer_.staState())
                      : nullptr;
  return prev_drvr_pin != nullptr;
}

bool SizeUpMatchGenerator::hasSingleStageFanout(sta::Pin* prev_drvr_pin,
                                                int& fanout) const
{
  sta::Vertex* prev_drvr_vertex
      = resizer_.graph()->pinDrvrVertex(prev_drvr_pin);
  if (prev_drvr_vertex == nullptr) {
    fanout = 0;
    return false;
  }

  fanout = 0;
  sta::VertexOutEdgeIterator edge_iter(prev_drvr_vertex, resizer_.graph());
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    if (!edge->isWire()) {
      continue;
    }
    ++fanout;
    if (fanout > 1) {
      return false;
    }
  }
  return true;
}

sta::LibertyCell* SizeUpMatchGenerator::selectReplacement(
    sta::LibertyCell* curr_cell,
    sta::Pin* prev_drvr_pin) const
{
  sta::LibertyPort* prev_drvr_port
      = resizer_.network()->libertyPort(prev_drvr_pin);
  sta::LibertyCell* prev_cell
      = prev_drvr_port != nullptr ? prev_drvr_port->libertyCell() : nullptr;
  return isSameFamilyStrongerDriver(curr_cell, prev_cell) ? prev_cell : nullptr;
}

bool SizeUpMatchGenerator::isSameFamilyStrongerDriver(
    sta::LibertyCell* curr_cell,
    sta::LibertyCell* prev_cell) const
{
  if (prev_cell == nullptr || prev_cell == curr_cell) {
    return false;
  }

  const bool same_family
      = (prev_cell->isBuffer() && curr_cell->isBuffer())
        || (prev_cell->isInverter() && curr_cell->isInverter());
  return same_family
         && resizer_.bufferDriveResistance(prev_cell)
                < resizer_.bufferDriveResistance(curr_cell);
}

}  // namespace rsz
