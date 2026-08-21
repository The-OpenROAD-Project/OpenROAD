// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "rcx/multiChipExtractor.h"

namespace rcx {

MultiChipExtractor::MultiChipExtractor(odb::dbDatabase* db, utl::Logger* logger)
    : db_{db}, logger_{logger}
{
}

void MultiChipExtractor::run(const ExtractOptions& /* options */)
{
  logger_->error(utl::RCX, 515, "3D extraction is not yet supported.");
}

void MultiChipExtractor::setExtractionRulesFile(
    odb::dbTech* tech,
    const std::string& extraction_rules_file)
{
  extraction_rules_files_[tech] = extraction_rules_file;
}

void MultiChipExtractor::setAssemblyExtractionRulesFile(
    const std::string& assembly_extraction_rules_file)
{
  assembly_extraction_rules_file_ = assembly_extraction_rules_file;
}

}  // namespace rcx
