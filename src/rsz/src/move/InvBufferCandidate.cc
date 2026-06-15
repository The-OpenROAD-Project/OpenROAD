// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "InvBufferCandidate.hh"

#include <cstdint>
#include <memory>
#include <string>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

InvBufferCandidate::InvBufferCandidate(Resizer& resizer,
                                       const Target& target,
                                       sta::Instance* buffer,
                                       sta::LibertyCell* inv_cell)
    : MoveCandidate(resizer, target), buffer_(buffer), inv_cell_(inv_cell)
{
}

MoveResult InvBufferCandidate::apply()
{
  sta::dbNetwork* db_network = resizer_.dbNetwork();
  sta::dbSta* sta = resizer_.sta();
  utl::Logger* logger = resizer_.logger();

  // === Pre-resolve everything before touching the netlist ===================
  // Once we delete the buffer the only safe thing apply() can do is finish
  // the inverter chain. All resolution that can fail (port lookups, pin
  // resolution, location queries) happens up front so a failure path returns
  // rejectedMove() without having mutated anything.

  sta::LibertyCell* orig_cell = db_network->libertyCell(buffer_);
  if (orig_cell == nullptr || !orig_cell->isBuffer()) {
    return rejectedMove();
  }

  sta::LibertyPort* orig_in_port = nullptr;
  sta::LibertyPort* orig_out_port = nullptr;
  orig_cell->bufferPorts(orig_in_port, orig_out_port);
  if (orig_in_port == nullptr || orig_out_port == nullptr) {
    return rejectedMove();
  }

  sta::LibertyPort* inv_in_port = nullptr;
  sta::LibertyPort* inv_out_port = nullptr;
  inv_cell_->bufferPorts(inv_in_port, inv_out_port);
  if (inv_in_port == nullptr || inv_out_port == nullptr) {
    return rejectedMove();
  }

  sta::Pin* buffer_in_pin = db_network->findPin(buffer_, orig_in_port);
  sta::Pin* buffer_out_pin = db_network->findPin(buffer_, orig_out_port);
  if (buffer_in_pin == nullptr || buffer_out_pin == nullptr) {
    return rejectedMove();
  }

  // Cache the flat dbNets and any hierarchical modnets on each side separately
  odb::dbNet* input_flat_db_net = db_network->flatNet(buffer_in_pin);
  odb::dbNet* output_flat_db_net = db_network->flatNet(buffer_out_pin);
  if (input_flat_db_net == nullptr || output_flat_db_net == nullptr) {
    return rejectedMove();
  }
  odb::dbModNet* input_hier_db_net = db_network->hierNet(buffer_in_pin);
  odb::dbModNet* output_hier_db_net = db_network->hierNet(buffer_out_pin);

  sta::Net* input_net = db_network->dbToSta(input_flat_db_net);
  sta::Net* output_net = db_network->dbToSta(output_flat_db_net);

  odb::dbInst* db_buffer = db_network->staToDb(buffer_);
  if (db_buffer == nullptr) {
    return rejectedMove();
  }

  // The new inverters and the mid net must live in the buffer's parent module,
  // not at the top, so hierarchy is preserved.
  sta::Instance* parent = db_network->getOwningInstanceParent(buffer_in_pin);

  // Buffer location (anchor when the upstream driver cannot be resolved)
  odb::Point buffer_loc;
  {
    int bx = 0;
    int by = 0;
    db_buffer->getLocation(bx, by);
    buffer_loc = {bx, by};
  }

  // Upstream driver location: prefer the prev-driver path the target already
  // carries; fall back to scanning the input net's drivers.
  odb::Point drvr_loc = buffer_loc;
  bool drvr_loc_found = false;
  if (const sta::Path* prev = target_.prevDriverPath(resizer_)) {
    if (sta::Vertex* v = prev->vertex(sta)) {
      const sta::Pin* prev_pin = v->pin();
      sta::Instance* prev_inst
          = prev_pin != nullptr ? db_network->instance(prev_pin) : nullptr;
      if (prev_inst != nullptr) {
        if (odb::dbInst* db_prev = db_network->staToDb(prev_inst)) {
          int dx = 0;
          int dy = 0;
          db_prev->getLocation(dx, dy);
          drvr_loc = {dx, dy};
          drvr_loc_found = true;
        }
      }
    }
  }
  if (!drvr_loc_found) {
    std::unique_ptr<sta::NetConnectedPinIterator> pin_iter(
        db_network->connectedPinIterator(input_net));
    while (pin_iter->hasNext()) {
      const sta::Pin* pin = pin_iter->next();
      if (pin == buffer_in_pin || !db_network->isDriver(pin)) {
        continue;
      }
      sta::Instance* drvr_inst = db_network->instance(pin);
      if (drvr_inst == nullptr) {
        continue;
      }
      if (odb::dbInst* db_drvr = db_network->staToDb(drvr_inst)) {
        int dx = 0;
        int dy = 0;
        db_drvr->getLocation(dx, dy);
        drvr_loc = {dx, dy};
        break;
      }
    }
  }

  // Load centroid for the second inverter
  int64_t lx_sum = 0;
  int64_t ly_sum = 0;
  int load_count = 0;
  {
    std::unique_ptr<sta::NetConnectedPinIterator> pin_iter(
        db_network->connectedPinIterator(output_net));
    while (pin_iter->hasNext()) {
      const sta::Pin* pin = pin_iter->next();
      if (pin == buffer_out_pin || !db_network->isLoad(pin)) {
        continue;
      }
      sta::Instance* load_inst = db_network->instance(pin);
      if (load_inst == nullptr) {
        continue;
      }
      if (odb::dbInst* db_load = db_network->staToDb(load_inst)) {
        int lxc = 0;
        int lyc = 0;
        db_load->getLocation(lxc, lyc);
        lx_sum += lxc;
        ly_sum += lyc;
        ++load_count;
      }
    }
  }
  const odb::Point load_centroid
      = (load_count > 0) ? odb::Point{static_cast<int>(lx_sum / load_count),
                                      static_cast<int>(ly_sum / load_count)}
                         : buffer_loc;

  // Interpolate inverter positions at 1/3 and 2/3 between upstream driver and
  // load centroid. Fall back to the buffer location as the upstream anchor
  // when prev driver did not resolve, so both inverters do not collapse to one
  // point.
  const odb::Point inv1_loc{
      drvr_loc.x() + (load_centroid.x() - drvr_loc.x()) / 3,
      drvr_loc.y() + (load_centroid.y() - drvr_loc.y()) / 3};
  const odb::Point inv2_loc{
      drvr_loc.x() + 2 * (load_centroid.x() - drvr_loc.x()) / 3,
      drvr_loc.y() + 2 * (load_centroid.y() - drvr_loc.y()) / 3};

  // Snapshot the buffer's name before deletion; we use it to derive base names
  // for the new inverters and mid net.
  const std::string buffer_name = db_network->name(buffer_);
  const std::string base_inv1 = buffer_name + "_inv1";
  const std::string base_inv2 = buffer_name + "_inv2";
  const std::string base_mid = buffer_name + "_mid_net";

  // Sig type for the new mid net so it matches the surrounding signal
  const odb::dbSigType mid_sig_type = input_flat_db_net->getSigType();

  // === Mutate ===============================================================
  // All netlist mutation goes through Resizer/dbNetwork helpers that the ECO
  // journal can replay. Resizer::makeInstance handles uniquification, sets the
  // placement status, legalizes the cell position, and increments the design
  // area; dbNetwork::makeNet handles net-name uniquification.

  sta->disconnectPin(buffer_in_pin);
  sta->disconnectPin(buffer_out_pin);
  sta->deleteInstance(buffer_);

  sta::Instance* inv1
      = resizer_.makeInstance(inv_cell_,
                              base_inv1.c_str(),
                              parent,
                              inv1_loc,
                              odb::dbNameUniquifyType::IF_NEEDED);
  sta::Instance* inv2
      = resizer_.makeInstance(inv_cell_,
                              base_inv2.c_str(),
                              parent,
                              inv2_loc,
                              odb::dbNameUniquifyType::IF_NEEDED);
  if (inv1 == nullptr || inv2 == nullptr) {
    return rejectedMove();
  }

  sta::Net* mid_net = db_network->makeNet(
      base_mid, parent, odb::dbNameUniquifyType::IF_NEEDED);
  if (odb::dbNet* mid_db_net = db_network->staToDb(mid_net)) {
    mid_db_net->setSigType(mid_sig_type);
  }

  sta->connectPin(inv1, inv_in_port, input_net);
  sta->connectPin(inv1, inv_out_port, mid_net);
  sta->connectPin(inv2, inv_in_port, mid_net);
  sta->connectPin(inv2, inv_out_port, output_net);

  // Re-attach hierarchical (mod) nets on the endpoints so write_verilog and
  // hierarchical STA see the same connectivity the original buffer had.
  // The mid net is purely local between the two inverters, so it needs no
  // modnet.
  if (input_hier_db_net != nullptr) {
    if (sta::Pin* inv1_in_pin = db_network->findPin(inv1, inv_in_port)) {
      if (odb::dbITerm* inv1_in_iterm = db_network->flatPin(inv1_in_pin)) {
        inv1_in_iterm->connect(input_hier_db_net);
      }
    }
  }
  if (output_hier_db_net != nullptr) {
    if (sta::Pin* inv2_out_pin = db_network->findPin(inv2, inv_out_port)) {
      if (odb::dbITerm* inv2_out_iterm = db_network->flatPin(inv2_out_pin)) {
        inv2_out_iterm->connect(output_hier_db_net);
      }
    }
  }

  // Topology of input_net, the new mid_net, and output_net changed; their
  // parasitic estimates must be rebuilt on the next STA update.
  resizer_.invalidateParasitics(input_net);
  resizer_.invalidateParasitics(mid_net);
  resizer_.invalidateParasitics(output_net);

  debugPrint(logger,
             RSZ,
             "inv_buffer_move",
             1,
             "ACCEPT InvBufferMove: Replaced buffer {} with two {} inverters",
             buffer_name,
             inv_cell_->name());

  return {
      .accepted = true,
      .type = MoveType::kInvBuffer,
      .move_count = 1,
      .touched_instances = {inv1, inv2},
  };
}

}  // namespace rsz
