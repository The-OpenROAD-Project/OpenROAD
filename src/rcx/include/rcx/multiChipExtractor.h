// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string>

#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "rcx/ext_options.h"
#include "utl/Logger.h"

namespace rcx {

class MultiChipExtractor
{
 public:
  MultiChipExtractor(odb::dbDatabase* db, utl::Logger* logger);

  void run(const ExtractOptions& options);

  void setExtractionRulesFile(odb::dbTech* tech,
                              const std::string& extraction_rules_file);
  void setAssemblyExtractionRulesFile(
      const std::string& assembly_extraction_rules_file);

 private:
  odb::dbDatabase* db_{nullptr};
  utl::Logger* logger_{nullptr};

  odb::PtrMap<odb::dbTech, std::string> extraction_rules_files_;
  std::string assembly_extraction_rules_file_;
};

}  // namespace rcx
