// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <boost/property_tree/json_parser.hpp>
#include <map>
#include <memory>
#include <vector>

#include "odb/db.h"
#include "utl/Logger.h"

namespace fin {

struct DensityFillLayerConfig;
class Graphics;

////////////////////////////////////////////////////////////////

// This class inserts metal fill to meet density rules according
// to the specification in a user given JSON file.
class DensityFill
{
 public:
  DensityFill(odb::dbDatabase* db, utl::Logger* logger, bool debug);
  ~DensityFill();

  DensityFill(const DensityFill&) = delete;
  DensityFill& operator=(const DensityFill&) = delete;
  DensityFill(const DensityFill&&) = delete;
  DensityFill& operator=(const DensityFill&&) = delete;

  void fill(const char* cfg_filename, const odb::Rect& fill_area);

 private:
  void loadConfig(const char* cfg_filename, odb::dbTech* tech);
  void readAndExpandLayers(odb::dbTech* tech,
                           boost::property_tree::ptree& tree);
  void fillLayer(odb::dbBlock* block,
                 odb::dbTechLayer* layer,
                 const odb::Rect& fill_bounds);

  odb::dbDatabase* db_;
  std::map<odb::dbTechLayer*, DensityFillLayerConfig> layers_;
  std::unique_ptr<Graphics> graphics_;
  utl::Logger* logger_;
};

}  // namespace fin
