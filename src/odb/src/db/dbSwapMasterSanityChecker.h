// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <string>

namespace utl {
class Logger;
}

namespace odb {

class dbModInst;
class dbModule;

class dbSwapMasterSanityChecker
{
 public:
  dbSwapMasterSanityChecker(dbModInst* new_mod_inst,
                            dbModule* src_module,
                            utl::Logger* logger);

  // Run all sanity checks. Returns total number of issues (warn + error).
  // Calls logger_->error() (throws) if any error-level issue was found.
  int run();

 private:
  int checkStructuralIntegrity();
  int checkPortPinMatching();
  int checkHierNetConnectivity();
  int checkFlatNetConnectivity();
  int checkInstanceHierarchy();
  int checkHashTableIntegrity();
  int checkNoDanglingObjects();
  int checkCombinationalLoops();

  // Log a warning-level issue (non-fatal)
  void warn(const std::string& msg);
  // Log an error-level issue (fatal — triggers logger_->error() at end of run)
  void error(const std::string& msg);

  // Context string for log messages: "module '<name>' (inst '<name>')"
  std::string masterContext() const;

  dbModInst* new_mod_inst_;
  dbModule* new_master_;
  dbModule* src_module_;
  dbModule* parent_;
  utl::Logger* logger_;
  int warn_count_ = 0;
  int error_count_ = 0;
};

}  // namespace odb
