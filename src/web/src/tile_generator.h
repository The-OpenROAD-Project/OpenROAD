// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/geom.h"

namespace sta {
class dbSta;
}

namespace utl {
class Logger;
}

namespace web {

struct Color;
class Search;

struct TileVisibility
{
  bool stdcells = true;
  bool macros = true;

  // Pad sub-types
  bool pad_input = true;
  bool pad_output = true;
  bool pad_inout = true;
  bool pad_power = true;
  bool pad_spacer = true;
  bool pad_areaio = true;
  bool pad_other = true;

  // Physical sub-types
  bool phys_fill = true;
  bool phys_endcap = true;
  bool phys_welltap = true;
  bool phys_tie = true;
  bool phys_antenna = true;
  bool phys_cover = true;
  bool phys_bump = true;
  bool phys_other = true;

  // Std cell sub-types (used when Liberty/STA is available)
  bool std_bufinv = true;
  bool std_bufinv_timing = true;
  bool std_clock_bufinv = true;
  bool std_clock_gate = true;
  bool std_level_shift = true;
  bool std_sequential = true;
  bool std_combinational = true;

  // Net sub-types (by dbSigType)
  bool net_signal = true;
  bool net_power = true;
  bool net_ground = true;
  bool net_clock = true;
  bool net_reset = true;
  bool net_tieoff = true;
  bool net_scan = true;
  bool net_analog = true;

  // Shapes
  bool routing = true;
  bool special_nets = true;
  bool pins = true;
  bool blockages = true;

  // Debug
  bool debug = false;

  bool isNetVisible(odb::dbNet* net) const
  {
    switch (net->getSigType().getValue()) {
      case odb::dbSigType::SIGNAL:
        return net_signal;
      case odb::dbSigType::POWER:
        return net_power;
      case odb::dbSigType::GROUND:
        return net_ground;
      case odb::dbSigType::CLOCK:
        return net_clock;
      case odb::dbSigType::RESET:
        return net_reset;
      case odb::dbSigType::TIEOFF:
        return net_tieoff;
      case odb::dbSigType::SCAN:
        return net_scan;
      case odb::dbSigType::ANALOG:
        return net_analog;
    }
    return true;
  }
};

class TileGenerator
{
 public:
  TileGenerator(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger);
  ~TileGenerator();

  bool hasSta() const { return sta_ != nullptr; }

  odb::Rect getBounds();

  std::vector<std::string> getRoutingLayers();

  std::vector<unsigned char> generateTile(const std::string& layer,
                                          int z,
                                          int x,
                                          int y,
                                          const TileVisibility& vis = {});

 private:
  void setPixel(std::vector<unsigned char>& image,
                int x,
                int y,
                const Color& c);

  static odb::Rect toPixels(double scale,
                            const odb::Rect& rect,
                            const odb::Rect& dbu_tile);

  void drawDebugOverlay(std::vector<unsigned char>& image,
                        int z,
                        int x,
                        int y);

  odb::dbDatabase* db_;
  sta::dbSta* sta_;
  utl::Logger* logger_;
  std::unique_ptr<Search> search_;
  static constexpr int kTileSizeInPixel = 256;
};

}  // namespace web
