// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include <filesystem>
#include <unordered_set>

#include "cgt/NetworkBuilder.h"
#include "cgt/RandomBits.h"
#include "db_sta/dbSta.hh"
#include "utl/deleter.h"

namespace abc {
using Abc_Ntk_t = struct Abc_Ntk_t_;
using Abc_Obj_t = struct Abc_Obj_t_;
using Vec_Ptr_t = struct Vec_Ptr_t_;
}  // namespace abc

namespace utl {
class Logger;
}

namespace odb {
class dbDatabase;
}  // namespace odb

namespace sta {
class dbSta;
}  // namespace sta

namespace lext {

class AbcLibrary;
class AbcLibraryFactory;

}  // namespace lext

namespace cgt {

using utl::Logger;

enum class GroupInstances
{
  No,
  CellPrefix,
  NetPrefix,
};

class ClockGating
{
 public:
  ClockGating();
  ~ClockGating();

  void init(utl::Logger* logger, sta::dbSta* open_sta);

  // Main function that performs the clock gating process
  void run();

  void addGateCondNet(const char* gate_cond)
  {
    gate_cond_nets_.insert(gate_cond);
  }
  void addInstance(const char* instance) { instances_.insert(instance); }
  void setMinInstances(size_t min_instances) { min_instances_ = min_instances; }
  void setMaxCover(size_t max_cover) { max_cover_ = max_cover; }
  void setGroupInstances(GroupInstances group_instances)
  {
    group_instances_ = group_instances;
  }
  void setDumpDir(const char* dir);

 private:
  // Searches for a minimal set of nets that can form a gating condition for the
  // given instances. The given range of candidate nets (begin-end) is divided
  // in two. If the first half can form a gating condition, the second half is
  // dropped from the list. Otherwise, the second half is recursed into.
  // Finally, the first half is recursed into.
  void searchClockGates(const std::vector<sta::Instance*>& instances,
                        std::vector<sta::Net*>& good_gate_conds,
                        std::vector<sta::Net*>::iterator begin,
                        std::vector<sta::Net*>::iterator end,
                        abc::Abc_Ntk_t& abc_network_ref,
                        bool clk_enable);
  // Exports a part of the network that contains the given instances and nets to
  // ABC.
  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> exportToAbc(
      const std::vector<sta::Instance*>& instances,
      const std::vector<sta::Net*>& gate_cond_nets);
  // Constructs an ABC network that will be used for testing if the given nets
  // are correct gating conditions.
  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> makeTestNetwork(
      const std::vector<sta::Instance*>& instances,
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
  bool isCorrectClockGate(const std::vector<sta::Instance*>& instances,
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

  using path = std::filesystem::path;
  path getNetworkGraphvizDumpPath(const char* name)
  {
    return getDumpDir()
           / (std::to_string(dump_counter_) + "_openroad_" + name + ".dot");
  }
  path getAbcGraphvizDumpPath(const char* name)
  {
    return getDumpDir()
           / (std::to_string(dump_counter_) + "_abc_" + name + ".dot");
  }
  path getAbcVerilogDumpPath(const char* name)
  {
    return getDumpDir()
           / (std::to_string(dump_counter_) + "_abc_" + name + ".v");
  }
  path getDumpDir()
  {
    if (dump_dir_) {
      return *dump_dir_;
    } else {
      logger_->error(
          utl::CGT, 10, "Dump directory is not set. Cannot create dump path.");
    }
  }
  std::string getUniqueName(const char* name)
  {
    // TODO: Avoid name collisions with pre-existing instances/nets
    auto& count = unique_names_[name];
    return std::string(name) + "_" + std::to_string(count++);
  }
  std::string getUniqueName(const char* prefix, const char* suffix)
  {
    return getUniqueName((std::string(prefix) + suffix).c_str());
  }
  std::unordered_map<std::string, size_t> unique_names_;

  // Determines if and how instances should be grouped.
  GroupInstances group_instances_ = GroupInstances::No;
  // Minimum number of instances that should be gated by one clock gate.
  size_t min_instances_ = 10;
  // Maximum number of nets that form the initial gating condition set for each
  // instance (group).
  size_t max_cover_ = 100;

  // Names of gating condition candidates specified by the user.
  std::unordered_set<std::string> gate_cond_nets_;
  // Names of instances to be gated specified by the user.
  std::unordered_set<std::string> instances_;
  // Path where dump files should be created.
  std::optional<path> dump_dir_;

  //
  size_t dump_counter_ = 0;
  size_t rejected_sim_count_ = 0;
  size_t rejected_sat_count_ = 0;
  size_t accepted_count_ = 0;
  size_t reuse_check_time_us_ = 0;
  size_t search_time_us_ = 0;
  size_t exist_check_time_us_ = 0;
  size_t network_export_time_us_ = 0;
  size_t network_build_time_us_ = 0;
  size_t sim_time_us_ = 0;
  size_t sat_time_us_ = 0;

  // Generates random bits (for random simulation).
  RandomBits rand_bits_;
  // Helper for inserting new instances/nets into a network.
  NetworkBuilder network_builder_;

  Logger* logger_;
  sta::dbSta* sta_;
  std::unique_ptr<lext::AbcLibraryFactory> abc_factory_;
  std::unique_ptr<lext::AbcLibrary> abc_library_;
};

}  // namespace cgt
