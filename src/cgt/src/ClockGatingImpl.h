// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "NetworkBuilder.h"
#include "RandomBits.h"
#include "cgt/ClockGating.h"
#include "db_sta/dbSta.hh"
#include "utl/Logger.h"
#include "utl/deleter.h"
#include "utl/unique_name.h"

namespace abc {
using Abc_Ntk_t = struct Abc_Ntk_t_;
using Abc_Obj_t = struct Abc_Obj_t_;
using Vec_Ptr_t = struct Vec_Ptr_t_;
}  // namespace abc

namespace odb {
class dbDatabase;
}  // namespace odb

namespace sta {
class dbSta;
}  // namespace sta

namespace cut {

class AbcLibrary;
class AbcLibraryFactory;

}  // namespace cut

namespace cgt {

class ClockGating::Impl
{
 public:
  Impl(utl::Logger* logger, sta::dbSta* open_sta);
  ~Impl();

  // Main function that performs the clock gating process
  void run();

  void setMinInstances(int min_instances) { min_instances_ = min_instances; }
  void setMaxCover(int max_cover) { max_cover_ = max_cover; }
  void setDumpDir(const char* dir);

 private:
  // Searches for a minimal set of nets that can form a gating condition for the
  // given instances. The given range of candidate nets (begin-end) is divided
  // in two. If the first half can form a gating condition, the second half is
  // dropped from the list. Otherwise, the second half is recursed into.
  // Finally, the first half is recursed into.
  void searchClockGates(sta::Instance* instance,
                        std::vector<sta::Net*>& good_gate_conds,
                        std::vector<sta::Net*>::iterator begin,
                        std::vector<sta::Net*>::iterator end,
                        abc::Abc_Ntk_t& abc_network_ref,
                        bool clk_enable);
  // Exports a part of the network that contains the given instances and nets to
  // ABC.
  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> exportToAbc(
      sta::Instance* instance,
      const std::vector<sta::Net*>& nets);
  // Constructs an ABC network that will be used for testing if the given nets
  // are correct gating conditions.
  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> makeTestNetwork(
      sta::Instance* instance,
      const std::vector<sta::Net*>& gate_cond_nets,
      abc::Abc_Ntk_t& abc_network_ref,
      bool clk_enable);
  // Performs a random simulation test on the given network and returns true if
  // no counterexample for the given gating conditions was found.
  bool simulationTest(abc::Abc_Ntk_t* abc_network,
                      const std::string& combined_gate_name);
  // Performs a SAT test on the given network and returns true if it is NOT
  // satisfiable (the given nets are correct gating conditions).
  bool satTest(abc::Abc_Ntk_t* abc_network,
               const std::string& combined_gate_name);
  // Returns true if the given gating conditions are correct for the given
  // clocked instances.
  bool isCorrectClockGate(sta::Instance* instance,
                          const std::vector<sta::Net*>& gate_cond_nets,
                          abc::Abc_Ntk_t& abc_network,
                          bool clk_enable);
  // Inserts a clock gate for the given clocked instances with the given gating
  // conditions into the network.
  void insertClockGate(const std::vector<sta::Instance*>& instances,
                       const std::vector<sta::Net*>& gate_cond_nets,
                       bool clk_enable);

  void dump(const char* name);
  void dumpAbc(const char* name, abc::Abc_Ntk_t* network);

  std::filesystem::path getNetworkGraphvizDumpPath(const char* name);
  std::filesystem::path getAbcGraphvizDumpPath(const char* name);
  std::filesystem::path getAbcVerilogDumpPath(const char* name);
  std::filesystem::path getDumpDir();

  // Minimum number of instances that should be gated by one clock gate.
  int min_instances_ = 10;
  // Maximum number of nets that form the initial gating condition set for each
  // instance.
  int max_cover_ = 100;

  // Path where dump files should be created.
  std::optional<std::filesystem::path> dump_dir_;

  // Debug counters and run time measurements
  int dump_counter_ = 0;
  int rejected_sim_count_ = 0;
  int rejected_sat_count_ = 0;
  int accepted_count_ = 0;
  double reuse_check_time_ = 0;
  double search_time_ = 0;
  double exist_check_time_ = 0;
  double network_export_time_ = 0;
  double network_build_time_ = 0;
  double sim_time_ = 0;
  double sat_time_ = 0;

  // For unique instance names
  utl::UniqueName unique_name_cond_;
  utl::UniqueName unique_name_cond_not_;
  utl::UniqueName unique_name_gate_;
  // Generates random bits (for random simulation).
  RandomBits rand_bits_;
  // Helper for inserting new instances/nets into a network.
  NetworkBuilder network_builder_;

  utl::Logger* logger_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  std::unique_ptr<cut::AbcLibraryFactory> abc_factory_;
  std::unique_ptr<cut::AbcLibrary> abc_library_;
};

}  // namespace cgt
