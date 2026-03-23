// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <string>

#include "spdlog/fmt/fmt.h"  // NOLINT(misc-include-cleaner)

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
  void checkStructuralIntegrity();
  void checkPortPinMatching();
  void checkHierNetConnectivity();
  void checkFlatNetConnectivity();
  void checkInstanceHierarchy();
  void checkHashTableIntegrity();
  void checkNoDanglingObjects();
  void checkCombinationalLoops();

  // Log a warning-level issue (non-fatal)
  template <typename... Args>
  void warn(const std::string& fmt_str, const Args&... args)
  {
    warnMsg(fmt::format(fmt::runtime(fmt_str), args...));
  }
  // Log an error-level issue (fatal — triggers logger_->error() at end of run)
  template <typename... Args>
  void error(const std::string& fmt_str, const Args&... args)
  {
    errorMsg(fmt::format(fmt::runtime(fmt_str), args...));
  }

  void warnMsg(const std::string& msg);
  void errorMsg(const std::string& msg);

  // Context string for log messages: "module '<name>' (inst '<name>')"
  std::string masterContext() const;

  dbModInst* new_mod_inst_{nullptr};
  dbModule* new_master_{nullptr};
  dbModule* src_module_{nullptr};
  dbModule* parent_{nullptr};
  utl::Logger* logger_{nullptr};
  int warn_count_{0};
  int error_count_{0};
};

}  // namespace odb
