// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
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

// Base fixture for tests that only manipulate an odb database. It has no
// dependency on OpenSTA/dbSta; tests that need timing should derive from
// tst::Fixture (tst/fixture.h) instead.
class DbFixture : public ::testing::Test
{
 protected:
  DbFixture();
  ~DbFixture() override;

  odb::dbDatabase* getDb() const { return db_.get(); }
  utl::Logger* getLogger() { return &logger_; }

  // In bazel this uses the runfiles mechanism to locate the file
  std::string getFilePath(const std::string& file_path) const;

  // Load a tech LEF file
  odb::dbTech* loadTechLef(const char* name, const std::string& lef_file);

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

  // Intended for use like: auto [n1, n2] = makeNets(block, "n1", "n2");
  template <typename... Args>
  std::array<odb::dbNet*, sizeof...(Args)> makeNets(odb::dbBlock* block,
                                                    Args... names)
  {
    return {odb::dbNet::create(block, names)...};
  }

  utl::Logger logger_;
  utl::UniquePtrWithDeleter<odb::dbDatabase> db_;
#ifdef BAZEL_BUILD
  std::unique_ptr<bazel::tools::cpp::runfiles::Runfiles> runfiles_;
#endif
};

}  // namespace tst
