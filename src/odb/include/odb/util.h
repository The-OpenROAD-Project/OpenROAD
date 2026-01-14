// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace utl {
class Logger;
}

namespace odb {
class dbBlock;
class dbBox;
class dbDatabase;
class dbNet;
class dbTechLayer;
class Rect;
class Polygon;

int makeSiteLoc(int x, double site_width, bool at_left_from_macro, int offset);

bool hasOneSiteMaster(dbDatabase* db);

void cutRows(dbBlock* block,
             int min_row_width,
             const std::vector<dbBox*>& blockages,
             int halo_x,
             int halo_y,
             utl::Logger* logger);

// Generates a string with the macro placement in mpl input format for
// individual macro placement
std::string generateMacroPlacementString(dbBlock* block);

void set_bterm_top_layer_grid(dbBlock* block,
                              dbTechLayer* layer,
                              int x_step,
                              int y_step,
                              Rect region,
                              int width,
                              int height,
                              int keepout);

bool dbHasCoreRows(dbDatabase* db);

class WireLengthEvaluator
{
 public:
  WireLengthEvaluator(dbBlock* block) : block_(block) {}
  int64_t hpwl() const;
  int64_t hpwl(int64_t& hpwl_x, int64_t& hpwl_y) const;
  void reportEachNetHpwl(utl::Logger* logger) const;
  void reportHpwl(utl::Logger* logger) const;

 private:
  int64_t hpwl(dbNet* net, int64_t& hpwl_x, int64_t& hpwl_y) const;

  dbBlock* block_;
};

}  // namespace odb
