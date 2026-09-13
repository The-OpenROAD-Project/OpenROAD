// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string>

#include "odb/db.h"
#include "rcx/extSpef.h"
#include "rcx/ext_options.h"
#include "utl/Logger.h"

namespace rcx {

class MultiChipSpefWriter
{
 public:
  MultiChipSpefWriter(odb::dbDatabase* db,
                      utl::Logger* logger,
                      const std::string& spef_version);

  void run(const SpefOptions& options);

 private:
  void setUnits(const SpefOptions& options);

  void writeChipSpef(odb::dbChip* chip, const SpefOptions& options);
  void writeInterChipSpef();
  std::string chipNetSpefString(odb::dbChipNet* chip_net);
  std::string bondNodeName(odb::dbChipCapNode* cap_node);

  odb::dbDatabase* db_{nullptr};
  utl::Logger* logger_{nullptr};

  std::string file_base_name_;
  SpefHeader spef_header_;

  ScaleFactors scale_factors_;
};

}  // namespace rcx
