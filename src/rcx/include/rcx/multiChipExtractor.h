// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>

#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "rcx/extRCap.h"
#include "rcx/ext_options.h"
#include "rcx/interChipModel.h"
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
  void loadRules();
  void extractChipParasitics(odb::dbChip* chip, const ExtractOptions& options);
  void extractInterChipParasitics();

  odb::dbDatabase* db_{nullptr};
  utl::Logger* logger_{nullptr};

  // Rules' files paths.
  odb::PtrMap<odb::dbTech, std::string> extraction_rules_files_;
  std::string assembly_extraction_rules_file_;

  // Rules' models i.e., the structures populated with rules file data.
  odb::PtrMap<odb::dbTech, std::unique_ptr<extRCModel>> tech_to_rules_model_;
  InterChipModel inter_chip_model_;

  const int corner_index_{0};
  const std::string corner_name_{"Typical"};
};

}  // namespace rcx
