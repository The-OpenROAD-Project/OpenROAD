// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "tcl.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

#ifdef BAZEL_BUILD
#include "tools/cpp/runfiles/runfiles.h"
#endif

namespace tst {

struct InstOptions
{
  struct ITermInfo
  {
    const char* net_name;   // created if needed
    const char* term_name;  // must match an existing dbMTerm
  };
  odb::dbRegion* region{nullptr};
  bool physical_only{false};
  odb::dbModule* parent_module{nullptr};
  odb::dbSourceType type{odb::dbSourceType::NONE};
  odb::Point location;
  odb::dbPlacementStatus status{odb::dbPlacementStatus::NONE};
  std::vector<ITermInfo> iterms;
};

struct BTermOptions
{
  struct BPinInfo
  {
    const char* layer_name;
    odb::Rect rect;
    odb::dbPlacementStatus status{odb::dbPlacementStatus::PLACED};
  };

  odb::dbIoType io_type{odb::dbIoType::INPUT};
  odb::dbSigType sig_type{odb::dbSigType::SIGNAL};
  std::vector<BPinInfo> bpins;
};

class Fixture : public ::testing::Test
{
 protected:
  Fixture();
  ~Fixture() override;

  odb::dbDatabase* getDb() const { return db_.get(); }
  sta::dbSta* getSta() const { return sta_.get(); }
  utl::Logger* getLogger() { return &logger_; }

  // In bazel this uses the runfiles mechanism to locate the file
  std::string getFilePath(const std::string& file_path) const;

  // if scene == nullptr, then a scene named "default" is used
  sta::LibertyLibrary* readLiberty(const std::string& filename,
                                   sta::Scene* scene = nullptr,
                                   const sta::MinMaxAll* min_max
                                   = sta::MinMaxAll::all(),
                                   bool infer_latches = false);

  // Load a tech LEF file
  odb::dbTech* loadTechLef(const char* name, const std::string& lef_file);

  // Load a library LEF file
  odb::dbLib* loadLibaryLef(odb::dbTech* tech,
                            const char* name,
                            const std::string& lef_file);
  // Load a tech + library LEF file
  odb::dbLib* loadTechAndLib(const char* tech_name,
                             const char* lib_name,
                             const std::string& lef_file);

  // Add macros to this library
  bool updateLib(odb::dbLib* lib, const std::string& lef_file);

  // Netlist editing helpers.
  //
  // These APIs are designed for maximum convenience rather than
  // performance.  The goal is to make this competitive with writing a
  // Verilog or DEF test case by hand.  C++ unit tests should only be
  // instantiating a small number of objects for readability and
  // focused testing.
  //
  // Name strings are used and objects are created if needed whenever
  // possible (e.g. referring to a net "foo" will find it or create it
  // if not present).

  odb::dbInst* makeInst(odb::dbBlock* block,
                        odb::dbMaster* master,
                        const char* name,
                        const InstOptions& options = {});

  // A net of the same name will be created if needed.
  odb::dbBTerm* makeBTerm(odb::dbBlock* block,
                          const char* name,
                          const BTermOptions& options = {});

  // Indented for use like: auto [n1, n2] = makeNets(block, {"n1", "n2"});
  template <int SZ>
  std::array<odb::dbNet*, SZ> makeNets(odb::dbBlock* block,
                                       const char* const (&names)[SZ])
  {
    std::array<odb::dbNet*, SZ> nets;
    for (int i = 0; i < SZ; ++i) {
      nets[i] = odb::dbNet::create(block, names[i]);
    }
    return nets;
  }

  utl::Logger logger_;
  utl::UniquePtrWithDeleter<odb::dbDatabase> db_;
  utl::UniquePtrWithDeleter<Tcl_Interp> interp_;
  std::unique_ptr<sta::dbSta> sta_;
#ifdef BAZEL_BUILD
  std::unique_ptr<bazel::tools::cpp::runfiles::Runfiles> runfiles_;
#endif
};

}  // namespace tst
