// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "utl/Logger.h"

namespace sta {
class LibertyCell;
class Network;
}  // namespace sta

namespace rsz {
class Resizer;
}

namespace syn {

class Graph;
class Net;

class Synthesis : public sta::dbStaState
{
 public:
  Synthesis(odb::dbDatabase* db,
            sta::dbSta* sta,
            rsz::Resizer* resizer,
            utl::Logger* logger);
  ~Synthesis() override;

  // Parse and elaborate SystemVerilog sources into the internal Graph.
  // Arguments are forwarded to slang (file paths, -I, --top, defines, etc.).
  bool svElaborate(const std::vector<std::string>& args);

  // Load graph from text IR file.
  void parse(const std::string& filename);

  // Remove Input/Output ports matching a glob pattern.
  void removePorts(const std::string& pattern);

  // Dump the graph in text IR format.
  void dump() const;
  void dump(const std::string& filename) const;

  // Dump the fanin cone of a net.
  void dumpFaninCone(const std::string& net_ref,
                     int max_depth,
                     bool stop_at_stateful) const;

  // Dump the fanout cone of a net.
  void dumpFanoutCone(const std::string& net_ref,
                      int max_depth,
                      bool stop_at_stateful) const;

  // Print cell statistics.
  void stats() const;

  // Print memory usage breakdown.
  void memoryUsage() const;

  // Mark all Name instances as tentative.
  void makeNamesTentative();

  // Per-bit DCE, splitting, and topological sort.
  void normalize();

  // Convert Other instances to Target where a matching LibertyCell exists.
  void importTargets();

  // Map Dff instances to library flip-flop cells (Target).
  void mapSequentials();

  // Constant folding and short-circuit optimization.
  void opt();

  // Split wide bitwise operations into per-bit fine operations.
  void bitblast(bool blast_arith = true);

  // Map combinational gates to library cells.
  void mapCombinationals();

  // Export AIG to ABC, run commands, reimport.
  void abcRoundtrip(const std::string& commands);

  // Post-mapping cell fusion optimization.
  void gateFuseOpt();

  // 4-valued liveness propagation: replace registers (and optionally any
  // combinational driver) whose value is provably never A with a constant.
  void livenessOpt(bool replace_combinational = false);

  // Export the mapped netlist to ODB.
  void exportToOdb();

  // Whether `cell` is on the don't-use list (forwarded from rsz::Resizer,
  // which mirrors liberty's dont_use plus any user-set entries).
  bool dontUse(const sta::LibertyCell* cell) const;

  Graph* graph() { return graph_.get(); }

 private:
  // Resolve a net reference string to a Net.
  Net resolveNetRef(const std::string& net_ref) const;

  odb::dbDatabase* db_ = nullptr;
  rsz::Resizer* resizer_ = nullptr;
  utl::Logger* logger_ = nullptr;
  std::unique_ptr<Graph> graph_;
};

// Free-function entry points re-declared here so tests don't need to
// pull in the private flow/ headers. The Synthesis methods of the same
// names forward to these.
void bitblast(Graph& g, bool blast_arith = true);
void mapSequentials(Graph& g,
                    sta::Network* network,
                    utl::Logger* logger,
                    const Synthesis& syn);
void mapCombinationals(Graph& g,
                       sta::Network* network,
                       utl::Logger* logger,
                       const Synthesis& syn);
void abcRoundtrip(Graph& g, const std::string& commands, utl::Logger* logger);
void livenessOpt(Graph& g,
                 utl::Logger* logger,
                 bool replace_combinational = false);

}  // namespace syn
