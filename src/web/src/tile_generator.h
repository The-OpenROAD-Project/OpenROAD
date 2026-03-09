// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "color.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace sta {
class dbSta;
}

namespace utl {
class Logger;
}

namespace web {

class Search;

struct ColoredRect
{
  odb::Rect rect;
  Color color;
  std::string layer;  // empty = draw on all layers
};

struct FlightLine
{
  odb::Point p1;
  odb::Point p2;
  Color color;
};

struct SelectionResult
{
  odb::dbInst* inst;
  std::string name;
  std::string master;
  odb::Rect bbox;
};

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

  // Blockages (dbBlockage / dbObstruction)
  bool placement_blockages = true;
  bool routing_obstructions = true;

  // Rows (off by default, matching GUI)
  bool rows = false;
  std::string raw_json_;  // stored for dynamic per-site lookups
  bool isSiteVisible(const std::string& site_name) const;

  // Tracks (off by default, matching GUI)
  bool tracks_pref = false;
  bool tracks_non_pref = false;

  // Debug
  bool debug = false;

  void parseFromJson(const std::string& json);

  bool isNetVisible(odb::dbNet* net) const;
  bool isInstVisible(odb::dbInst* inst, sta::dbSta* sta) const;
};

class TileGenerator
{
 public:
  TileGenerator(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger);
  ~TileGenerator();

  bool hasSta() const { return sta_ != nullptr; }
  sta::dbSta* getSta() const { return sta_; }

  odb::Rect getBounds();

  std::vector<std::string> getLayers();
  std::vector<std::string> getSites();

  std::vector<SelectionResult> selectAt(int dbu_x,
                                        int dbu_y,
                                        int zoom = 0,
                                        const TileVisibility& vis = {});

  odb::dbBlock* getBlock() const;

  std::vector<unsigned char> generateTile(
      const std::string& layer,
      int z,
      int x,
      int y,
      const TileVisibility& vis = {},
      const std::vector<odb::Rect>& highlight_rects = {},
      const std::vector<ColoredRect>& colored_rects = {},
      const std::vector<FlightLine>& flight_lines = {});

 private:
  void setPixel(std::vector<unsigned char>& image,
                int x,
                int y,
                const Color& c);

  static odb::Rect toPixels(double scale,
                            const odb::Rect& rect,
                            const odb::Rect& dbu_tile);

  void drawDebugOverlay(std::vector<unsigned char>& image, int z, int x, int y);

  void drawHighlight(std::vector<unsigned char>& image,
                     const std::vector<odb::Rect>& rects,
                     const odb::Rect& dbu_tile,
                     double scale);

  void drawColoredHighlight(std::vector<unsigned char>& image,
                            const std::vector<ColoredRect>& rects,
                            const std::string& current_layer,
                            const odb::Rect& dbu_tile,
                            double scale);

  void drawFlightLines(std::vector<unsigned char>& image,
                       const std::vector<FlightLine>& lines,
                       const odb::Rect& dbu_tile,
                       double scale);

  static void blendPixel(std::vector<unsigned char>& image,
                         int x,
                         int y,
                         const Color& c);

  static void drawLine(std::vector<unsigned char>& image,
                       int x0,
                       int y0,
                       int x1,
                       int y1,
                       const Color& c);

  odb::dbDatabase* db_;
  sta::dbSta* sta_;
  utl::Logger* logger_;
  std::unique_ptr<Search> search_;
  static constexpr int kTileSizeInPixel = 256;
};

struct TimingPathSummary;

std::pair<odb::dbITerm*, odb::dbBTerm*> resolvePin(odb::dbBlock* block,
                                                   const std::string& pin_name);

void collectNetShapes(odb::dbNet* net,
                      odb::dbITerm* drv_iterm,
                      odb::dbBTerm* drv_bterm,
                      odb::dbITerm* snk_iterm,
                      odb::dbBTerm* snk_bterm,
                      const Color& color,
                      std::vector<ColoredRect>& rects,
                      std::vector<FlightLine>& lines);

void collectTimingPathShapes(odb::dbBlock* block,
                             const TimingPathSummary& path,
                             std::vector<ColoredRect>& rects,
                             std::vector<FlightLine>& lines);

}  // namespace web
