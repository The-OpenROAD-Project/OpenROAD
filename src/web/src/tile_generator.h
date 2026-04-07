// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <any>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "color.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace sta {
class dbSta;
}

namespace gui {
class HeatMapDataSource;
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
  std::any object;  // dbInst*, dbNet*, etc.
  std::string name;
  std::string type_name;  // "Inst", "Net", etc.
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
  bool pin_markers = true;
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

  void eagerInit();
  bool shapesReady() const;

  bool hasSta() const { return sta_ != nullptr; }
  sta::dbSta* getSta() const { return sta_; }

  odb::Rect getBounds() const;
  int getPinMaxSize() const;

  std::vector<std::string> getLayers() const;
  std::vector<std::string> getSites() const;

  std::vector<SelectionResult> selectAt(
      int dbu_x,
      int dbu_y,
      int zoom = 0,
      const TileVisibility& vis = {},
      const std::set<std::string>& visible_layers = {});

  struct SnapResult
  {
    std::pair<odb::Point, odb::Point> edge;
    int distance = 0;
    bool found = false;
  };

  SnapResult snapAt(int dbu_x,
                    int dbu_y,
                    int search_radius,
                    int point_snap_threshold,
                    bool horizontal,
                    bool vertical,
                    const TileVisibility& vis,
                    const std::set<std::string>& visible_layers) const;

  odb::dbBlock* getBlock() const;
  odb::dbChip* getChip() const;

  std::vector<unsigned char> generateTile(
      const std::string& layer,
      int z,
      int x,
      int y,
      const TileVisibility& vis = {},
      const std::vector<odb::Rect>& highlight_rects = {},
      const std::vector<odb::Polygon>& highlight_polys = {},
      const std::vector<ColoredRect>& colored_rects = {},
      const std::vector<FlightLine>& flight_lines = {},
      const std::map<uint32_t, Color>* module_colors = nullptr,
      const std::set<uint32_t>* focus_net_ids = nullptr,
      const std::set<uint32_t>* route_guide_net_ids = nullptr) const;
  std::vector<unsigned char> generateHeatMapTile(gui::HeatMapDataSource& source,
                                                 int z,
                                                 int x,
                                                 int y) const;

  // Render full design (or region) to a PNG file.  Works without a running
  // web server.  region in DBU; if zero-area, defaults to die + 5% margin.
  void saveImage(const std::string& filename,
                 const odb::Rect& region,
                 int width_px,
                 double dbu_per_pixel,
                 const TileVisibility& vis) const;

 private:
  // Render a single tile into a raw RGBA buffer (pre-PNG-encoding).
  // Same signature as generateTile but returns raw pixels.
  std::vector<unsigned char> renderTileBuffer(
      const std::string& layer,
      int z,
      int x,
      int y,
      const TileVisibility& vis = {},
      const std::vector<odb::Rect>& highlight_rects = {},
      const std::vector<odb::Polygon>& highlight_polys = {},
      const std::vector<ColoredRect>& colored_rects = {},
      const std::vector<FlightLine>& flight_lines = {},
      const std::map<uint32_t, Color>* module_colors = nullptr,
      const std::set<uint32_t>* focus_net_ids = nullptr,
      const std::set<uint32_t>* route_guide_net_ids = nullptr) const;
  void setPixel(std::vector<unsigned char>& image,
                int x,
                int y,
                const Color& c) const;

  void drawDebugOverlay(std::vector<unsigned char>& image,
                        int z,
                        int x,
                        int y) const;

  static int getBitmapTextWidth(std::string_view text, int scale);
  static int getBitmapTextHeight(int scale);
  static void drawBitmapText(std::vector<unsigned char>& image,
                             int x,
                             int y,
                             std::string_view text,
                             int scale,
                             const Color& color);
  // Draw text rotated 90° CCW (reads bottom-to-top).
  // (x, y) is the bottom-left corner of the rotated text block.
  static void drawBitmapTextRotated(std::vector<unsigned char>& image,
                                    int x,
                                    int y,
                                    std::string_view text,
                                    int scale,
                                    const Color& color);

  void drawHighlight(std::vector<unsigned char>& image,
                     const std::vector<odb::Rect>& rects,
                     const std::vector<odb::Polygon>& polys,
                     const odb::Rect& dbu_tile,
                     double scale) const;

  void drawColoredHighlight(std::vector<unsigned char>& image,
                            const std::vector<ColoredRect>& rects,
                            const std::string& current_layer,
                            const odb::Rect& dbu_tile,
                            double scale) const;

  void drawFlightLines(std::vector<unsigned char>& image,
                       const std::vector<FlightLine>& lines,
                       const odb::Rect& dbu_tile,
                       double scale) const;

  void drawRouteGuides(std::vector<unsigned char>& image,
                       const std::set<uint32_t>& net_ids,
                       const std::string& layer,
                       const Color& color,
                       const odb::Rect& dbu_tile,
                       double scale) const;

  static odb::Rect toPixels(double scale,
                            const odb::Rect& rect,
                            const odb::Rect& dbu_tile);

  void fillPolygon(std::vector<unsigned char>& image,
                   const odb::Polygon& poly,
                   const odb::Rect& dbu_tile,
                   double scale,
                   const Color& color,
                   bool blend = false) const;

  void drawFilledRect(std::vector<unsigned char>& buffer,
                      const odb::Rect& rect,
                      const Color& color) const;

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
