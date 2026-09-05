// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "request_handler.h"

#include <fnmatch.h>

#include <algorithm>
#include <any>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

#include "boost/json/array.hpp"
#include "boost/json/object.hpp"
#include "boost/json/serialize.hpp"
#include "boost/json/value.hpp"
#include "cli_completer.h"
#include "clock_tree_report.h"
#include "color.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gui/descriptor_registry.h"
#include "gui/gui.h"
#include "gui/heatMap.h"
#include "hierarchy_report.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "request_dispatcher.h"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "tile_generator.h"
#include "timing_report.h"
#include "utl/Logger.h"
#include "utl/algorithms.h"

namespace web {

//------------------------------------------------------------------------------
// ShapeCollector — a gui::Painter that collects rectangles, polygons and
// lines from descriptor->highlight() calls for use in tile rendering.
// Lines matter for nets: an unrouted net's highlight (and the Qt GUI's
// NetWithSink sink path) is drawn as flight lines, which used to be
// silently dropped here.
//------------------------------------------------------------------------------

class ShapeCollector : public gui::Painter
{
 public:
  ShapeCollector() : Painter(nullptr, odb::Rect(), 1.0) {}

  std::vector<odb::Rect> rects;
  std::vector<odb::Polygon> polys;
  std::vector<FlightLine> lines;

  void drawRect(const odb::Rect& rect, int, int) override
  {
    rects.push_back(rect);
  }
  void drawPolygon(const odb::Polygon& polygon) override
  {
    polys.push_back(polygon);
  }
  void drawOctagon(const odb::Oct& oct) override { polys.emplace_back(oct); }
  void drawLine(const odb::Point& p1, const odb::Point& p2) override
  {
    // Selection-highlight yellow (matches the rect/poly highlight color).
    lines.push_back(
        {.p1 = p1, .p2 = p2, .color = {.r = 255, .g = 255, .b = 0, .a = 255}});
  }

  // No-ops
  Color getPenColor() override { return {}; }
  void setPen(odb::dbTechLayer*, bool) override {}
  void setPen(const Color&, bool, int) override {}
  void setPenWidth(int) override {}
  void setBrush(odb::dbTechLayer*, int) override {}
  void setBrush(const Color&, const Brush&) override {}
  void setFont(const Font&) override {}
  void saveState() override {}
  void restoreState() override {}
  void drawCircle(int, int, int) override {}
  void drawX(int, int, int) override {}
  void drawPolygon(const std::vector<odb::Point>&) override {}
  void drawString(int, int, Anchor, const std::string&, bool) override {}
  odb::Rect stringBoundaries(int, int, Anchor, const std::string&) override
  {
    return {};
  }
  void drawRuler(int, int, int, int, bool, const std::string&) override {}
};

namespace {

// Read a JSON array of strings as a std::set.  Used for the visible_layers
// field which arrives as ["metal1", "metal2", ...].  Throws when the value
// is not an array of strings — that's a contract violation.
std::set<std::string> arrayAsStringSet(const boost::json::value& v)
{
  std::set<std::string> out;
  for (const auto& item : v.as_array()) {
    out.emplace(item.as_string());
  }
  return out;
}

// Build a JSON-array bbox: [xMin, yMin, xMax, yMax].
boost::json::array bboxArray(const odb::Rect& r)
{
  return boost::json::array{r.xMin(), r.yMin(), r.xMax(), r.yMax()};
}

// Build a JSON-array RGBA: [r, g, b, a].
boost::json::array colorArray(const gui::Painter::Color& c)
{
  return boost::json::array{c.r, c.g, c.b, c.a};
}

// Serialize a boost::json::value into a WebSocketResponse's payload.
void writePayload(WebSocketResponse& resp, const boost::json::value& v)
{
  std::string s = boost::json::serialize(v);
  resp.payload.assign(s.begin(), s.end());
}

// Read a JSON number as a double, accepting the integer kinds too.  JS
// serializes whole numbers without a decimal point (5.0 -> "5"), so Boost.JSON
// parses them as int64/uint64 and value::as_double() would throw.
double jsonToDouble(const boost::json::value& v)
{
  if (v.is_double()) {
    return v.get_double();
  }
  if (v.is_int64()) {
    return static_cast<double>(v.get_int64());
  }
  if (v.is_uint64()) {
    return static_cast<double>(v.get_uint64());
  }
  throw std::runtime_error("value is not a number");
}

}  // namespace

// RAII helper: temporarily sets Descriptor::Property::convert_dbu /
// convert_string to micron-aware converters (matching the Qt GUI's
// defaults) for the lifetime of the scope.  When `use_dbu` is true a
// raw-DBU identity formatter and integer parser are installed instead.
// convert_string must always be replaced: the library default returns 0
// without setting the ok flag, so descriptor editors that parse
// coordinates (e.g. dbInst X/Y) would silently reject every edit.
// Must be held while the sta_lock mutex is held — the global statics are
// not otherwise thread-safe.
//
// The scale comes from the database, not from a block: a multi-die design's
// top chip is hierarchical and owns no dbBlock, so keying off one would leave
// its properties as unlabelled raw integers.  dbDatabase::getDbuPerMicron() is
// the design-wide value every tech and block shares, and is 0 only before any
// LEF is read.
class [[nodiscard]] ScopedDbuFormat
{
 public:
  ScopedDbuFormat(odb::dbDatabase* db, bool use_dbu)
      : saved_to_string_(gui::Descriptor::Property::convert_dbu),
        saved_to_dbu_(gui::Descriptor::Property::convert_string)
  {
    if (use_dbu || db == nullptr || db->getDbuPerMicron() == 0) {
      gui::Descriptor::Property::convert_string
          = [](const std::string& value, bool* ok) {
              return parseValue(value, /*dbu_per_micron=*/0.0, ok);
            };
      return;  // keep default formatter (raw DBU)
    }
    const double dbu_per_micron = db->getDbuPerMicron();
    const int precision = dbuPrecision(dbu_per_micron);
    gui::Descriptor::Property::convert_dbu
        = [dbu_per_micron, precision](int value, bool add_units) {
            auto str = utl::to_numeric_string(
                static_cast<double>(value) / dbu_per_micron, precision);
            if (add_units) {
              str += " \xC2\xB5m";  // UTF-8 µm
            }
            return str;
          };
    gui::Descriptor::Property::convert_string
        = [dbu_per_micron](const std::string& value, bool* ok) {
            return parseValue(value, dbu_per_micron, ok);
          };
  }
  ~ScopedDbuFormat()
  {
    gui::Descriptor::Property::convert_dbu = saved_to_string_;
    gui::Descriptor::Property::convert_string = saved_to_dbu_;
  }
  ScopedDbuFormat(const ScopedDbuFormat&) = delete;
  ScopedDbuFormat& operator=(const ScopedDbuFormat&) = delete;

 private:
  // Mirrors MainWindow::convertStringToDBU: strip a trailing unit
  // (anything after a space, 'u', or UTF-8 'µ'/'μ'), then parse as an
  // integer DBU count (dbu_per_micron == 0) or a micron double.
  static int parseValue(const std::string& value,
                        double dbu_per_micron,
                        bool* ok)
  {
    std::string text = value;
    for (const std::string_view sep :
         {std::string_view(" "),
          std::string_view("u"),
          std::string_view("\xC2\xB5"),     // µ (micro sign)
          std::string_view("\xCE\xBC")}) {  // μ (greek mu)
      if (const auto pos = text.find(sep); pos != std::string::npos) {
        text = text.substr(0, pos);
        break;
      }
    }
    try {
      std::size_t consumed = 0;
      if (dbu_per_micron <= 0.0) {
        const int dbu = std::stoi(text, &consumed);
        *ok = consumed == text.size();
        return *ok ? dbu : 0;
      }
      const double microns = std::stod(text, &consumed);
      *ok = consumed == text.size();
      return *ok ? static_cast<int>(std::lround(microns * dbu_per_micron)) : 0;
    } catch (const std::exception&) {
      *ok = false;
      return 0;
    }
  }

  gui::DBUToString saved_to_string_;
  gui::StringToDBU saved_to_dbu_;
};
// Clamp + quantize the client's devicePixelRatio so the server renders tiles
// at a stable 256*dpr and the tile cache has few buckets.  Snaps to the common
// HiDPI ratios; anything outside [1,3] or non-finite falls back to 1.0.
// Normalize the client-reported device pixel ratio.
//
// Only the browser knows the display's ratio, so this is the value the client
// sent, not one the server picks.  It is honoured rather than snapped to a
// ladder: the ratio decides the rendered tile's pixel size, and a client whose
// ratio is not on the ladder gets a tile of the wrong size that the browser
// then rescales — which is the moiré beat the whole tile pipeline is built to
// avoid.  A 1.75 or 2.5 display (ordinary Windows scale factors) hit that.
//
// Rounded to two decimals only to bound the tile-cache key space, since dpr is
// part of the key: without it a ratio like 1.3333333 would fork its own set of
// cached tiles for every distinct trailing digit.  In practice this is one
// bucket per connected client.
//
// MUST match quantizeDpr() in tile-request.js — the client sizes its merged
// canvas from its own copy, and any disagreement resamples every tile.
static double quantizeDpr(const double raw)
{
  if (!std::isfinite(raw) || raw <= 1.0) {
    return 1.0;
  }
  const double clamped = std::min(raw, 3.0);
  return std::round(clamped * 100.0) / 100.0;
}

// The exact device-pixel side length the client will display the tile in.
//
// Sent explicitly rather than derived from dpr here: a tile's CSS box is only a
// whole number of device pixels when tileSize*dpr is an integer, and where it
// is not (256 CSS px is 426.67 device px at a 1.6667 display scale) any size
// this end picks is one the browser has to resample.  The client knows the box
// it will use, so it names the pixel count.
//
// Clamped so a malformed request cannot ask for a gigantic buffer — the render
// allocates tile_px*supersample squared.  0 (absent or unusable) means "not
// specified"; the generator falls back to 256*dpr.
static int quantizeTilePx(const double raw)
{
  if (!std::isfinite(raw) || raw <= 0.0) {
    return 0;
  }
  // Clamped BEFORE rounding: std::llround of a double past the range of long
  // long is undefined, and `raw` is whatever the client sent.
  constexpr double kMinTilePx = 32.0;
  constexpr double kMaxTilePx = 2048.0;
  return static_cast<int>(std::lround(std::clamp(raw, kMinTilePx, kMaxTilePx)));
}

std::string assetPathFromTarget(const std::string_view target)
{
  std::string path(target);
  // Query first, then fragment: a fragment is never sent by a browser, but a
  // hand-built request can carry one and it must not reach the lookup either.
  const size_t query = path.find('?');
  if (query != std::string::npos) {
    path.resize(query);
  }
  const size_t fragment = path.find('#');
  if (fragment != std::string::npos) {
    path.resize(fragment);
  }
  if (path.empty() || path == "/") {
    return "/index.html";
  }
  return path;
}

WebSocketResponse errorResponse(const uint32_t id,
                                const std::string_view message)
{
  WebSocketResponse resp;
  resp.id = id;
  resp.type = WebSocketResponse::kError;
  resp.payload.assign(message.begin(), message.end());
  return resp;
}

bool parseTileCoords(const boost::json::object& json,
                     int& z,
                     int& x,
                     int& y,
                     std::string& error)
{
  const auto read = [&json, &error](const char* key, int& out) {
    const auto* v = json.if_contains(key);
    if (v == nullptr) {
      error = std::string("missing \"") + key + "\"";
      return false;
    }
    // is_int64() and not as_int64(): a null (what a non-finite client-side
    // number serializes to) or a fractional zoom must be reported as the
    // contract violation it is, not converted.
    if (!v->is_int64()) {
      error = std::string("\"") + key + "\" must be an integer";
      return false;
    }
    const int64_t value = v->get_int64();
    if (value < std::numeric_limits<int>::min()
        || value > std::numeric_limits<int>::max()) {
      error = std::string("\"") + key + "\" is out of range";
      return false;
    }
    out = static_cast<int>(value);
    return true;
  };

  if (!read("z", z) || !read("x", x) || !read("y", y)) {
    return false;
  }
  if (z < 0 || z > kMaxTileZoom) {
    error = "\"z\" is outside [0, " + std::to_string(kMaxTileZoom) + "]";
    return false;
  }
  // x and y are deliberately NOT range-checked against the 2^z grid.  Leaflet
  // routinely asks for tiles off the edge of the world -- whenever the design
  // is smaller than the viewport, or while panning at the border -- and the
  // renderers answer those with a transparent tile.  Rejecting them turns
  // ordinary panning into a stream of errors: a first cut of this check
  // produced 336 refusals in one session, all of them for legitimate
  // off-grid tiles at zoom 0.
  return true;
}

// Payload for the {"type":"refresh"} broadcast sent after a DB-mutating
// request.  Carries the CURRENT design bounds: edits can change them
// (moving an instance outside the block bbox, deleting an edge instance),
// which shifts the tile georeference — clients must resync their
// coordinate transforms or every later click/highlight lands offset.
// Bounds use the same [[yMin,xMin],[yMax,xMax]] order as the bounds
// response.
static std::string refreshBroadcastPayload(const TileGenerator& gen)
{
  const odb::Rect bounds = gen.getBounds();
  boost::json::object o;
  o["type"] = "refresh";
  o["bounds"]
      = boost::json::array{boost::json::array{bounds.yMin(), bounds.xMin()},
                           boost::json::array{bounds.yMax(), bounds.xMax()}};
  return boost::json::serialize(o);
}

// Store a Selected in the clickables vector and return its index.
static int storeSelectable(std::vector<gui::Selected>& selectables,
                           const gui::Selected& sel)
{
  int id = static_cast<int>(selectables.size());
  selectables.push_back(sel);
  return id;
}

static void serializeAnyValue(boost::json::object& out,
                              std::string_view field_name,
                              const std::any& value,
                              std::vector<gui::Selected>& selectables,
                              bool short_name = false)
{
  if (auto* sel = std::any_cast<gui::Selected>(&value)) {
    if (*sel) {
      const std::string name
          = short_name ? sel->getShortName() : sel->getName();
      int id = storeSelectable(selectables, *sel);
      out[field_name] = name;
      out[std::string(field_name) + "_select_id"] = id;
      return;
    }
  }
  out[field_name] = gui::Descriptor::Property::toString(value);
}

// Describe a property's editor for the client.  Mirrors the Qt
// EditorItemDelegate rules: a non-empty options list renders as a combo
// ("list", committed by option index); a bool value renders as a
// True/False choice; otherwise the input kind is inferred from the
// property's current value type (getEditorType).
static boost::json::object serializeEditor(
    const gui::Descriptor::Editor& editor,
    const std::any& value)
{
  boost::json::object o;
  if (!editor.options.empty()) {
    o["type"] = "list";
    boost::json::array options;
    options.reserve(editor.options.size());
    for (const auto& option : editor.options) {
      options.emplace_back(option.name);
    }
    o["options"] = std::move(options);
    return o;
  }
  if (std::any_cast<bool>(&value) != nullptr) {
    o["type"] = "bool";
  } else if (std::any_cast<int>(&value) != nullptr
             || std::any_cast<unsigned int>(&value) != nullptr
             || std::any_cast<double>(&value) != nullptr
             || std::any_cast<float>(&value) != nullptr) {
    o["type"] = "number";
  } else {
    o["type"] = "string";
  }
  return o;
}

// Serialize an ordered list of unkeyed values (Property values held as a
// std::vector/std::set of std::any) as inspector children.  Mirrors the Qt
// inspector's makeList(): entries have no name of their own, so they are
// numbered 1..N and the value carries the content.
template <typename Container>
static boost::json::array serializeAnyList(
    const Container& values,
    std::vector<gui::Selected>& selectables)
{
  boost::json::array children;
  children.reserve(values.size());
  int index = 0;
  for (const auto& value : values) {
    boost::json::object child;
    child["name"] = std::to_string(++index);
    serializeAnyValue(child, "value", value, selectables);
    children.emplace_back(std::move(child));
  }
  return children;
}

// Serialize a PropertyTable as a row/column grid.  The Qt inspector renders
// these as an HTML table; the client does the same from this data rather than
// from server-generated markup.  Headers are kept separate from the cells so
// the client can drop the header row or column when a table has none.
static boost::json::object serializePropertyTable(
    const gui::PropertyTable& table)
{
  boost::json::object out;

  boost::json::array columns;
  columns.reserve(table.getColumnHeaders().size());
  for (const auto& header : table.getColumnHeaders()) {
    columns.emplace_back(header);
  }
  out["column_headers"] = std::move(columns);

  boost::json::array rows;
  rows.reserve(table.getRowHeaders().size());
  for (const auto& header : table.getRowHeaders()) {
    rows.emplace_back(header);
  }
  out["row_headers"] = std::move(rows);

  boost::json::array data;
  data.reserve(table.getData().size());
  for (const auto& row : table.getData()) {
    boost::json::array cells;
    cells.reserve(row.size());
    for (const auto& cell : row) {
      cells.emplace_back(cell);
    }
    data.emplace_back(std::move(cells));
  }
  out["data"] = std::move(data);

  return out;
}

static boost::json::object serializeProperty(
    const gui::Descriptor::Property& prop,
    std::vector<gui::Selected>& selectables,
    const gui::Descriptor::Editors* editors = nullptr)
{
  boost::json::object o;
  o["name"] = prop.name;
  if (editors != nullptr) {
    if (const auto it = editors->find(prop.name); it != editors->end()) {
      o["editable"] = true;
      o["editor"] = serializeEditor(it->second, prop.value);
    }
  }

  if (auto* plist = std::any_cast<gui::Descriptor::PropertyList>(&prop.value)) {
    boost::json::array children;
    children.reserve(plist->size());
    for (const auto& [key, val] : *plist) {
      boost::json::object child;
      serializeAnyValue(child, "name", key, selectables, /*short_name=*/true);
      serializeAnyValue(child, "value", val, selectables);
      children.emplace_back(std::move(child));
    }
    o["children"] = std::move(children);
  } else if (auto* sel_set = std::any_cast<gui::SelectionSet>(&prop.value)) {
    boost::json::array children;
    children.reserve(sel_set->size());
    for (const auto& sel : *sel_set) {
      boost::json::object child;
      int id = storeSelectable(selectables, sel);
      child["name"] = sel.getName();
      child["name_select_id"] = id;
      children.emplace_back(std::move(child));
    }
    o["children"] = std::move(children);
  } else if (auto* table = std::any_cast<gui::PropertyTable>(&prop.value)) {
    o["table"] = serializePropertyTable(*table);
  } else if (auto* vec = std::any_cast<std::vector<std::any>>(&prop.value)) {
    o["children"] = serializeAnyList(*vec, selectables);
  } else if (auto* set = std::any_cast<std::set<std::any>>(&prop.value)) {
    o["children"] = serializeAnyList(*set, selectables);
  } else if (auto* sel = std::any_cast<gui::Selected>(&prop.value)) {
    if (*sel) {
      int id = storeSelectable(selectables, *sel);
      o["value"] = sel->getName();
      o["value_select_id"] = id;
    }
  } else {
    o["value"] = prop.toString();
  }
  return o;
}

// The GUI draws the driver×sink product straight to the screen; here the
// lines are materialized in SessionState and copied per overlay tile, so
// cap the product to keep memory bounded on huge INOUT/fanout nets
// (beyond this the fan is visual mush anyway).
constexpr size_t kMaxFlywires = 4096;

// Mirrors the flywire fallback of gui::NetDescriptor::highlight
// (dbDescriptors.cpp:1766-1825): straight driver->sink lines between the
// net's placed terminals.  When `term_boxes` is non-null, the terminal
// pin boxes are collected in the same pass (used by the flywires-only
// mode, which suppresses the descriptor's wire shapes).  Supply and routed
// special nets get terminal boxes but no lines, matching the GUI.
static void collectNetFlightLines(odb::dbNet* net,
                                  std::vector<FlightLine>& lines,
                                  std::vector<odb::Rect>* term_boxes = nullptr)
{
  if (!net) {
    return;
  }
  // GUI parity: neither supply nets nor routed special nets get flywires;
  // their terminal boxes are still highlighted.
  const bool is_supply = net->getSigType().isSupply();
  const bool lines_enabled
      = !is_supply && (!net->isSpecial() || net->getFirstSWire() == nullptr);
  if (!lines_enabled && term_boxes == nullptr) {
    return;  // nothing left to collect
  }

  std::set<odb::Point> driver_locs;
  std::set<odb::Point> sink_locs;
  // GUI parity: the ITerms of a supply net are not highlighted at all
  // (DbNetDescriptor::highlight guards its ITerm loop with !is_supply), while
  // its BTerms are — hence the asymmetry with the BTerm loop below.
  if (!is_supply) {
    for (odb::dbITerm* iterm : net->getITerms()) {
      if (!iterm->getInst()->getPlacementStatus().isPlaced()) {
        continue;
      }
      if (term_boxes) {
        term_boxes->push_back(iterm->getBBox());
      }
      if (!lines_enabled) {
        continue;
      }
      odb::Point center;
      int x = 0;
      int y = 0;
      if (iterm->getAvgXY(&x, &y)) {
        center = odb::Point(x, y);
      } else if (odb::dbBox* bbox = iterm->getInst()->getBBox()) {
        const odb::Rect rect = bbox->getBox();
        center = odb::Point((rect.xMax() + rect.xMin()) / 2,
                            (rect.yMax() + rect.yMin()) / 2);
      } else {
        int ix = 0;
        int iy = 0;
        iterm->getInst()->getLocation(ix, iy);
        center = odb::Point(ix, iy);
      }
      const auto iotype = iterm->getIoType();
      if (iotype == odb::dbIoType::INPUT || iotype == odb::dbIoType::INOUT) {
        sink_locs.insert(center);
      }
      if (iotype == odb::dbIoType::OUTPUT || iotype == odb::dbIoType::INOUT) {
        driver_locs.insert(center);
      }
    }
  }
  for (odb::dbBTerm* bterm : net->getBTerms()) {
    const auto iotype = bterm->getIoType();
    // IO direction is from the block's perspective: an INPUT bterm
    // drives the net (GUI is_source_bterm / is_sink_bterm).
    const bool driver_term = iotype == odb::dbIoType::INPUT
                             || iotype == odb::dbIoType::INOUT
                             || iotype == odb::dbIoType::FEEDTHRU;
    const bool sink_term = iotype == odb::dbIoType::OUTPUT
                           || iotype == odb::dbIoType::INOUT
                           || iotype == odb::dbIoType::FEEDTHRU;
    for (odb::dbBPin* pin : bterm->getBPins()) {
      const odb::Rect rect = pin->getBBox();
      if (term_boxes) {
        term_boxes->push_back(rect);
      }
      if (!lines_enabled) {
        continue;
      }
      const odb::Point center((rect.xMax() + rect.xMin()) / 2,
                              (rect.yMax() + rect.yMin()) / 2);
      if (sink_term) {
        sink_locs.insert(center);
      }
      if (driver_term) {
        driver_locs.insert(center);
      }
    }
  }
  if (!lines_enabled) {
    return;
  }

  for (const odb::Point& driver : driver_locs) {
    for (const odb::Point& sink : sink_locs) {
      if (lines.size() >= kMaxFlywires) {
        return;
      }
      lines.push_back(
          FlightLine{.p1 = driver, .p2 = sink, .color = kSelectionYellow});
    }
  }
}

// Collect a net's special (geometric) routing shapes.  The GUI draws these
// unconditionally at the tail of DbNetDescriptor::highlight, i.e. also in
// flywires-only mode, so the flywire branch below has to reproduce them.
//
// Reproduced rather than delegated to DbNetDescriptor because the descriptor
// cannot be made to honor the mode from here: its gate is
// painter.getOptions()->isFlywireHighlightOnly(), and gui::Options is a private
// Qt-dependent interface (QColor/QFont), so a non-Qt module has no way to
// implement it — ShapeCollector's null Options resolves to DefaultOptions,
// which answers false.  Going through Gui::getDescriptor<odb::dbSWire*>() for
// just the tail is no better: an unregistered descriptor makes
// DescriptorRegistry emit GUI-0053, which is fatal, and nothing registers the
// odb descriptors in the web unit tests.
//
// The shapes match DbSBoxDescriptor::highlight (dbDescriptors.cpp:5707).
static void collectSpecialWireShapes(odb::dbNet* net,
                                     std::vector<odb::Rect>& rects,
                                     std::vector<odb::Polygon>& polys)
{
  if (!net) {
    return;
  }
  for (odb::dbSWire* swire : net->getSWires()) {
    for (odb::dbSBox* box : swire->getWires()) {
      if (box->getDirection() == odb::dbSBox::OCTILINEAR) {
        polys.emplace_back(box->getOct());
      } else {
        rects.push_back(box->getBox());
      }
    }
  }
}

// Append one selection's highlight shapes.  For nets, honor the
// "Flywires only" toggle (GUI isFlywireHighlightOnly()): replace the
// routed wire/guide shapes with driver->sink flight lines.  With the
// toggle off, unrouted nets still get flywires (GUI fallback — the
// descriptor emits them via painter.drawLine, which ShapeCollector
// drops, so they are recomputed here).
static void appendHighlightShapes(const gui::Selected& sel,
                                  std::vector<odb::Rect>& rects,
                                  std::vector<odb::Polygon>& polys,
                                  std::vector<FlightLine>& lines,
                                  const bool flywires_only)
{
  if (!sel) {
    return;
  }
  // Set when the flywire fan was collected here, so the descriptor's own
  // copy of it is not appended on top.
  bool own_flywires = false;
  // Pointer-form any_cast: DbNetDescriptor::isNet() is true for BOTH of
  // its payload types (dbNet* and NetWithSink) — the value-form cast
  // would throw std::bad_any_cast on the latter.  Non-dbNet* payloads
  // fall through to the plain descriptor highlight below.
  odb::dbNet* const* net_ptr = std::any_cast<odb::dbNet*>(&sel.getObject());
  if (sel.isNet() && net_ptr) {
    odb::dbNet* net = *net_ptr;
    if (flywires_only) {
      // Only the routed wire/guide shapes are suppressed.  Terminal pin boxes
      // and special (geometric) routing stay visible, because the GUI draws
      // them regardless of the flywire mode — without the SWires a selected
      // supply net would come back with no highlight at all.
      collectNetFlightLines(net, lines, &rects);
      collectSpecialWireShapes(net, rects, polys);
      return;
    }
    if (net && !net->getWire() && net->getGuides().empty()) {
      // Unrouted net: DbNetDescriptor::highlight draws the driver->sink fan
      // itself (draw_flywires stays true with no wire and no guides), and
      // ShapeCollector captures drawLine, so taking both would emit the fan
      // twice.  Collect it here instead of from the descriptor: this path
      // honours kMaxFlywires, which the descriptor does not.
      collectNetFlightLines(net, lines);
      own_flywires = true;
    }
  }
  ShapeCollector collector;
  sel.highlight(collector);
  rects.insert(rects.end(), collector.rects.begin(), collector.rects.end());
  polys.insert(polys.end(), collector.polys.begin(), collector.polys.end());
  if (!own_flywires) {
    lines.insert(lines.end(), collector.lines.begin(), collector.lines.end());
  }
}

static void collectHighlightShapes(const gui::Selected& sel,
                                   std::vector<odb::Rect>& rects,
                                   std::vector<odb::Polygon>& polys,
                                   std::vector<FlightLine>& lines,
                                   const bool flywires_only)
{
  rects.clear();
  polys.clear();
  lines.clear();
  appendHighlightShapes(sel, rects, polys, lines, flywires_only);
}

// Return the 0-based position of the iterator within the selection set,
// or -1 if the set is empty.  Mirrors Qt GUI's
// Inspector::getSelectedIteratorPosition().
static int selectionIteratorPosition(const gui::SelectionSet& set,
                                     gui::SelectionSet::const_iterator itr)
{
  if (set.empty() || itr == set.end()) {
    return -1;
  }
  return static_cast<int>(std::distance(set.begin(), itr));
}

// Accumulate highlight shapes from all items in a selection set.
static void collectMultiHighlightShapes(const gui::SelectionSet& selections,
                                        std::vector<odb::Rect>& rects,
                                        std::vector<odb::Polygon>& polys,
                                        std::vector<FlightLine>& lines,
                                        const bool flywires_only)
{
  rects.clear();
  polys.clear();
  lines.clear();
  for (const auto& sel : selections) {
    appendHighlightShapes(sel, rects, polys, lines, flywires_only);
  }
}

// Report whether the selection set contains any instance / any net, so the
// frontend context menu can enable/disable items by type (mirrors the Qt
// LayoutViewer::updateContextMenuItems + Gui::anyObjectInSet).
static void addSelectionTypeFlags(boost::json::object& root,
                                  const gui::SelectionSet& selection)
{
  bool has_inst = false;
  bool has_net = false;
  for (const auto& s : selection) {
    if (!s) {
      continue;
    }
    if (s.isInst()) {
      has_inst = true;
    } else if (s.isNet()) {
      has_net = true;
    }
    if (has_inst && has_net) {
      break;
    }
  }
  root["sel_has_inst"] = has_inst;
  root["sel_has_net"] = has_net;
}

// Qt parity: MainWindow::selectHighlightConnectedNets (Ctrl+LeftClick).
// Expands the selection with SIGNAL nets attached to the selected
// instances' ITerms (OUTPUT/INPUT/INOUT; FEEDTHRU excluded).  v1 inserts
// the plain net for input pins too: NetWithSink lives in a gui-private
// header under CMake, so the driver->sink flight line is a follow-up.
static int addConnectedNets(gui::SelectionSet& selection_set)
{
  // Snapshot the instances first: nets are inserted into the same set,
  // and odb deletes std::less on db pointers so the selection set itself
  // (ordered by Selected) is the deduplicator.
  std::vector<odb::dbInst*> insts;
  for (const auto& sel : selection_set) {
    if (!sel || !sel.isInst()) {
      continue;
    }
    const auto* inst_ptr = std::any_cast<odb::dbInst*>(&sel.getObject());
    if (inst_ptr && *inst_ptr) {
      insts.push_back(*inst_ptr);
    }
  }
  auto* registry = gui::DescriptorRegistry::instance();
  int added = 0;
  for (odb::dbInst* inst : insts) {
    for (odb::dbITerm* iterm : inst->getITerms()) {
      odb::dbNet* net = iterm->getNet();
      if (!net || net->getSigType() != odb::dbSigType::SIGNAL) {
        continue;
      }
      const auto io = iterm->getIoType();
      if (io != odb::dbIoType::OUTPUT && io != odb::dbIoType::INPUT
          && io != odb::dbIoType::INOUT) {
        continue;
      }
      gui::Selected net_sel = registry->makeSelected(net);
      if (net_sel && selection_set.insert(net_sel).second) {
        ++added;
      }
    }
  }
  return added;
}

// Descriptor actions that must not be surfaced in the web client:
// - "Insert Buffer" / "Copy to layer" construct Qt dialogs (guarded by
//   ENABLE_QT in dbDescriptors.cpp); in a Qt-enabled binary running in
//   web mode triggering them would crash — there is no QApplication.
// - Focus / route-guide / zoom actions call global gui::Gui methods that
//   are stub no-ops in web builds; the web inspector already provides
//   per-session equivalents in its toolbar.
// - Tracks / timing-cone / timing actions have no web renderer yet
//   (follow-ups); their gui::Gui calls are stub no-ops.
// Any new descriptor action not listed here appears automatically.
static const std::set<std::string, std::less<>> kSuppressedActions = {
    "Insert Buffer",
    "Copy to layer",
    "Focus",
    "De-focus",
    "Show Route Guides",
    "Hide Route Guides",
    "Show Tracks",
    "Hide Tracks",
    "Fanin Cone",
    "Fanout Cone",
    "Timing",
    "Zoom to",
};

// Run the previously inspected object's reserved "deselect" action (a
// cleanup callback, e.g. tearing down a timing cone) when inspection
// moves to a different object.  Mirrors Qt's Inspector::inspect().
// Must be called under the STA lock, before current_inspected is
// overwritten, and never with a stale (destroyed) old_sel.
static void runDeselectAction(const gui::Selected& old_sel,
                              const gui::Selected& new_sel)
{
  if (!old_sel || old_sel == new_sel) {
    return;
  }
  try {
    // getDescriptorActions(): Selected::getActions() lives in the full
    // gui library (its "Zoom to" needs Gui::get()); the web only links
    // gui_descriptors and blocklists "Zoom to" anyway.
    for (const auto& action : old_sel.getDescriptorActions()) {
      if (action.name == gui::Descriptor::kDeselectAction) {
        action.callback();
        return;
      }
    }
  } catch (const std::exception&) {
    // Cleanup is best-effort; never let it break the request.
  }
}

// Returns the highlight group holding `sel`, or -1 when not highlighted.
// Requires state.selection_mutex to be held.
static int highlightGroupOfLocked(const SessionState& state,
                                  const gui::Selected& sel)
{
  if (!sel) {
    return -1;
  }
  for (int group = 0; group < gui::kNumHighlightSet; ++group) {
    const auto& members = state.highlight_groups[group];
    if (members.find(sel) != members.end()) {
      return group;
    }
  }
  return -1;
}

// The three highlight vectors and the source tag form one invariant — always
// mutate them through these helpers (callers hold selection_mutex).  The tag
// must follow whether anything was actually collected: claiming a source for an
// empty selection would let a later flywires_only flip re-derive (and so
// resurrect) a highlight the user never had.
static void clearSelectionHighlights(SessionState& state)
{
  state.highlight_rects.clear();
  state.highlight_polys.clear();
  state.highlight_lines.clear();
  state.highlight_source = SessionState::HighlightSource::kNone;
}

static void setSelectionHighlights(SessionState& state,
                                   const gui::Selected& sel)
{
  collectHighlightShapes(sel,
                         state.highlight_rects,
                         state.highlight_polys,
                         state.highlight_lines,
                         state.flywires_only);
  state.highlight_source = sel ? SessionState::HighlightSource::kInspected
                               : SessionState::HighlightSource::kNone;
}

static void setSelectionSetHighlights(SessionState& state)
{
  collectMultiHighlightShapes(state.selection_set,
                              state.highlight_rects,
                              state.highlight_polys,
                              state.highlight_lines,
                              state.flywires_only);
  state.highlight_source = state.selection_set.empty()
                               ? SessionState::HighlightSource::kNone
                               : SessionState::HighlightSource::kSelectionSet;
}

// Rebuild the per-group overlay shapes from the group members.  Colors
// come from the Qt GUI's palette (gui::Painter::kHighlightColors,
// translucent fill); filled=true reuses the DRC/timing colored-rect
// rendering (blend fill + solid outline).  Goes through appendHighlightShapes,
// not sel.highlight() directly, so a group member honours "Flywires only" the
// same way the current selection does.  Requires state.selection_mutex and
// the STA lock (sel.highlight() calls descriptor code).
static void rebuildHighlightGroupShapesLocked(SessionState& state)
{
  state.highlight_group_rects.clear();
  state.highlight_group_polys.clear();
  state.highlight_group_lines.clear();
  for (int group = 0; group < gui::kNumHighlightSet; ++group) {
    if (state.highlight_groups[group].empty()) {
      continue;
    }
    const auto& qt_color = gui::Painter::kHighlightColors[group];
    const Color color{.r = static_cast<uint8_t>(qt_color.r),
                      .g = static_cast<uint8_t>(qt_color.g),
                      .b = static_cast<uint8_t>(qt_color.b),
                      .a = static_cast<uint8_t>(qt_color.a)};
    for (const auto& sel : state.highlight_groups[group]) {
      if (!sel) {
        continue;
      }
      std::vector<odb::Rect> rects;
      std::vector<odb::Polygon> polys;
      std::vector<FlightLine> lines;
      appendHighlightShapes(sel, rects, polys, lines, state.flywires_only);
      for (const auto& rect : rects) {
        state.highlight_group_rects.push_back(
            {.rect = rect, .color = color, .layer = "", .filled = true});
      }
      for (const auto& poly : polys) {
        state.highlight_group_polys.push_back({.poly = poly, .color = color});
      }
      // Members whose highlight() draws lines (e.g. unrouted nets) would
      // otherwise be invisible in the overlay; tint them with the group.
      for (const auto& line : lines) {
        state.highlight_group_lines.push_back(
            {.p1 = line.p1, .p2 = line.p2, .color = color});
      }
    }
  }
}

// Erase `sel` from every highlight group.  Returns true if it was a
// member of any.  Requires state.selection_mutex to be held.
static bool removeFromHighlightGroupsLocked(SessionState& state,
                                            const gui::Selected& sel)
{
  bool removed = false;
  for (auto& members : state.highlight_groups) {
    removed |= members.erase(sel) > 0;
  }
  return removed;
}

bool consumeStaleSelection(SessionState& state)
{
  if (!state.selection_stale.exchange(false)) {
    return false;
  }
  {
    std::lock_guard<std::mutex> lock(state.selection_mutex);
    state.current_inspected = gui::Selected();
    state.navigation_history.clear();
    state.selection_set.clear();
    state.selection_itr = state.selection_set.end();
    clearSelectionHighlights(state);
    state.hover_rects.clear();
    for (auto& members : state.highlight_groups) {
      members.clear();
    }
    state.highlight_group_rects.clear();
    state.highlight_group_polys.clear();
    state.highlight_group_lines.clear();
  }
  {
    std::lock_guard<std::mutex> lock(state.selectables_mutex);
    state.selectables.clear();
  }
  return true;
}

// `use_dbu` is only used to label the debug line: the lengths themselves go
// through Descriptor::Property::convert_dbu, which the caller's
// ScopedDbuFormat has already pointed at microns or at raw DBU.  Reusing that
// converter is what keeps the log in the same unit the inspector is showing.
//
// highlight_group is the 0-based color group holding `sel` (-1 = not
// highlighted), computed by the caller under selection_mutex via
// highlightGroupOfLocked — this function runs outside that lock.
static void writeInspectPayload(boost::json::object& o,
                                const gui::Selected& sel,
                                std::vector<gui::Selected>& new_selectables,
                                bool can_navigate_back,
                                bool use_dbu,
                                utl::Logger* logger,
                                int highlight_group = -1,
                                const odb::dbTransform& world_xfm = {})
{
  o["can_navigate_back"] = can_navigate_back ? 1 : 0;
  if (!sel) {
    o["error"] = "invalid select_id";
    debugPrint(
        logger, utl::WEB, "select", 1, "inspect outline: invalid select_id");
    return;
  }

  auto props = sel.getProperties();
  o["name"] = sel.getName();
  o["type"] = sel.getTypeName();
  o["highlight_group"] = highlight_group;
  const gui::Descriptor::Editors editors = sel.getEditors();
  boost::json::array prop_arr;
  prop_arr.reserve(props.size());
  for (const auto& prop : props) {
    prop_arr.emplace_back(serializeProperty(prop, new_selectables, &editors));
  }
  o["properties"] = std::move(prop_arr);

  boost::json::array actions;
  for (const auto& action : sel.getDescriptorActions()) {
    if (action.name == gui::Descriptor::kDeselectAction
        || kSuppressedActions.find(action.name) != kSuppressedActions.end()) {
      continue;
    }
    actions.emplace_back(action.name);
  }
  if (!actions.empty()) {
    o["actions"] = std::move(actions);
  }

  odb::Rect bbox;
  const bool has_bbox = sel.getBBox(bbox);
  if (has_bbox) {
    // A gui::Descriptor reports the bbox in the object's OWN block
    // coordinates, but the client draws in root/world space — the same space
    // selectAt already puts SelectionResult::bbox in.  Lift it through the
    // chiplet's local-to-root transform so an object inside a translated or
    // rotated dbChipInst is outlined where it is actually drawn.  Identity for
    // single-die designs, and for callers with no chiplet context (see the
    // world_xfm default).
    world_xfm.apply(bbox);
    o["bbox"] = bboxArray(bbox);
  }

  // This payload is what the client draws the yellow dashed selection outline
  // from (inspector.js highlightBBox): it needs both a bbox and a type other
  // than "Inst" — instances get the tile-rendered yellow highlight instead.
  // Log what the client will see so a missing or misplaced outline can be
  // attributed to the server side.
  if (has_bbox) {
    const auto len = [](const int dbu) {
      return gui::Descriptor::Property::convert_dbu(dbu, /*add_units=*/false);
    };
    const char* unit = use_dbu ? "dbu" : "um";
    debugPrint(logger,
               utl::WEB,
               "select",
               1,
               "inspect outline: type={} name={} bbox_{}=({},{})-({},{}) "
               "size_{}={}x{} props={} dashed_outline={} back={}",
               sel.getTypeName(),
               sel.getName(),
               unit,
               len(bbox.xMin()),
               len(bbox.yMin()),
               len(bbox.xMax()),
               len(bbox.yMax()),
               unit,
               len(bbox.dx()),
               len(bbox.dy()),
               props.size(),
               // Mirrors inspector.js's `data.type !== 'Inst'` literally, so
               // the log cannot drift from what the client actually draws.
               sel.getTypeName() != "Inst",
               can_navigate_back);
  } else {
    debugPrint(logger,
               utl::WEB,
               "select",
               1,
               "inspect outline: type={} name={} no bbox, no outline drawn "
               "(props={}, back={})",
               sel.getTypeName(),
               sel.getName(),
               props.size(),
               can_navigate_back);
  }

  if (sel.isNet()) {
    // Pointer-form cast: the payload may be NetWithSink (see
    // appendHighlightShapes) — the value-form cast would throw.
    if (odb::dbNet* const* net = std::any_cast<odb::dbNet*>(&sel.getObject());
        net && *net && !(*net)->getGuides().empty()) {
      o["has_guides"] = 1;
    }
  }
}

// Shared inspection-response trailer: writes the inspected object's payload
// plus the selection cursor fields into `root`, and installs the freshly
// derived selectables under selectables_mutex.  Handlers add their own
// ok/error/deleted fields around this call.
static void writeInspectTrailer(boost::json::object& root,
                                SessionState& state,
                                const gui::Selected& sel,
                                bool can_navigate_back,
                                bool use_dbu,
                                utl::Logger* logger,
                                int hl_group,
                                int sel_count,
                                int sel_index)
{
  std::vector<gui::Selected> new_selectables;
  writeInspectPayload(
      root, sel, new_selectables, can_navigate_back, use_dbu, logger, hl_group);
  root["selection_count"] = static_cast<int64_t>(sel_count);
  root["selection_index"] = static_cast<int64_t>(sel_index);
  std::lock_guard<std::mutex> lock(state.selectables_mutex);
  state.selectables = std::move(new_selectables);
}

static boost::json::object serializeHeatMapOption(
    const gui::HeatMapDataSource::MapSetting& option)
{
  boost::json::object o;
  if (std::holds_alternative<gui::HeatMapDataSource::MapSettingBoolean>(
          option)) {
    const auto& setting
        = std::get<gui::HeatMapDataSource::MapSettingBoolean>(option);
    o["type"] = "bool";
    o["name"] = setting.name;
    o["label"] = setting.label;
    o["value"] = setting.getter();
  } else {
    const auto& setting
        = std::get<gui::HeatMapDataSource::MapSettingMultiChoice>(option);
    o["type"] = "choice";
    o["name"] = setting.name;
    o["label"] = setting.label;
    o["value"] = setting.getter();
    boost::json::array choices;
    for (const auto& choice : setting.choices()) {
      choices.emplace_back(choice);
    }
    o["choices"] = std::move(choices);
  }
  return o;
}

static boost::json::object serializeHeatMap(gui::HeatMapDataSource& source,
                                            const bool active)
{
  if (active) {
    source.ensureMap();
  }
  const bool populated = source.isPopulated();
  const bool has_data = source.hasData();

  boost::json::object o;
  o["name"] = source.getShortName();
  o["title"] = source.getName();
  o["active"] = active;
  o["settings_group"] = source.getSettingsGroupName();
  o["has_data"] = has_data;
  o["can_adjust_grid"] = source.canAdjustGrid();
  o["show_numbers"] = source.getShowNumbers();
  o["show_legend"] = source.getShowLegend();
  o["supports_numbers"] = true;
  o["units"] = source.getValueUnits();
  o["display_range_increment"] = source.getDisplayRangeIncrement();
  o["display_min"] = source.convertPercentToValue(source.getDisplayRangeMin());
  o["display_max"] = source.convertPercentToValue(source.getDisplayRangeMax());
  o["display_min_limit"]
      = source.convertPercentToValue(source.getDisplayRangeMinimumValue());
  o["display_max_limit"]
      = source.convertPercentToValue(source.getDisplayRangeMaximumValue());
  o["draw_below_min"] = source.getDrawBelowRangeMin();
  o["draw_above_max"] = source.getDrawAboveRangeMax();
  o["log_scale"] = source.getLogScale();
  o["reverse_log"] = source.getReverseLogScale();
  o["grid_x"] = source.getGridXSize();
  o["grid_y"] = source.getGridYSize();
  o["grid_min"] = source.getGridSizeMinimumValue();
  o["grid_max"] = source.getGridSizeMaximumValue();
  o["alpha"] = source.getColorAlpha();
  o["alpha_min"] = source.getColorAlphaMinimum();
  o["alpha_max"] = source.getColorAlphaMaximum();
  o["bounds"] = bboxArray(source.getBounds());

  boost::json::array options;
  for (const auto& option : source.getMapSettings()) {
    options.emplace_back(serializeHeatMapOption(option));
  }
  o["options"] = std::move(options);

  boost::json::array legend;
  if (populated) {
    const auto& generator = source.getColorGenerator();
    const int color_count = generator.getColorCount();
    for (const auto& [color_index, color_value] : source.getLegendValues()) {
      boost::json::object entry;
      entry["value"] = source.formatValue(color_value, true);
      const gui::Painter::Color color
          = generator.getColor(100.0 * color_index / std::max(1, color_count),
                               source.getColorAlpha());
      entry["color"] = colorArray(color);
      legend.emplace_back(std::move(entry));
    }
  }
  o["legend"] = std::move(legend);
  return o;
}

static std::string buildHeatMapsPayloadLocked(SessionState& state)
{
  boost::json::object root;
  root["active"] = state.active_heatmap;
  boost::json::array heatmaps;
  for (const auto& [name, source] : state.heatmaps) {
    heatmaps.emplace_back(
        serializeHeatMap(*source, name == state.active_heatmap));
  }
  root["heatmaps"] = std::move(heatmaps);
  return boost::json::serialize(root);
}

WebSocketResponse TileHandler::serializeBounds(const uint32_t id,
                                               const TileGenerator& gen)
{
  WebSocketResponse resp;
  resp.id = id;
  resp.type = WebSocketResponse::kJson;
  writePayload(resp, serializeBoundsResponse(gen, gen.shapesReady()));
  return resp;
}

WebSocketResponse TileHandler::serializeTech(const uint32_t id,
                                             const TileGenerator& gen)
{
  WebSocketResponse resp;
  resp.id = id;
  resp.type = WebSocketResponse::kJson;
  writePayload(resp, serializeTechResponse(gen));
  return resp;
}

WebSocketResponse TileHandler::renderTile(
    const uint32_t id,
    const std::string& layer,
    const int z,
    const int x,
    const int y,
    const TileVisibility& vis,
    const TileGenerator& gen,
    const std::vector<odb::Rect>& highlight_rects,
    const std::vector<odb::Polygon>& highlight_polys,
    const std::vector<ColoredRect>& colored_rects,
    const std::vector<FlightLine>& flight_lines,
    const std::map<uint32_t, Color>* module_colors,
    const std::set<uint32_t>* focus_net_ids,
    const std::set<uint32_t>* route_guide_net_ids,
    const double dpr,
    const int tile_px)
{
  WebSocketResponse resp;
  resp.id = id;
  resp.type = WebSocketResponse::kPng;
  resp.payload = gen.generateTile(layer,
                                  z,
                                  x,
                                  y,
                                  vis,
                                  highlight_rects,
                                  highlight_polys,
                                  colored_rects,
                                  flight_lines,
                                  module_colors,
                                  focus_net_ids,
                                  route_guide_net_ids,
                                  dpr,
                                  tile_px);
  return resp;
}

//------------------------------------------------------------------------------
// SelectHandler
//------------------------------------------------------------------------------

SelectHandler::SelectHandler(std::shared_ptr<TileGenerator> gen,
                             std::shared_ptr<TclEvaluator> tcl_eval)
    : gen_(std::move(gen)), tcl_eval_(std::move(tcl_eval))
{
}

void SelectHandler::registerRequests(RequestDispatcher& d)
{
  d.add("select",
        WebSocketRequest::kSelect,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSelect(req, state);
        });
  d.add("inspect",
        WebSocketRequest::kInspect,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleInspect(req, state);
        });
  d.add("inspect_back",
        WebSocketRequest::kInspectBack,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleInspectBack(req, state);
        });
  d.add("hover",
        WebSocketRequest::kHover,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleHover(req, state);
        });
  d.add("snap",
        WebSocketRequest::kSnap,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleSnap(req);
        });
  d.add("schematic_cone",
        WebSocketRequest::kSchematicCone,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleSchematicCone(req);
        });
  d.add("schematic_full",
        WebSocketRequest::kSchematicFull,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleSchematicFull(req);
        });
  d.add("schematic_inspect",
        WebSocketRequest::kSchematicInspect,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSchematicInspect(req, state);
        });
  d.add("select_next",
        WebSocketRequest::kSelectNext,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSelectNext(req, state);
        });
  d.add("select_prev",
        WebSocketRequest::kSelectPrev,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSelectPrev(req, state);
        });
  d.add("set_property",
        WebSocketRequest::kSetProperty,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSetProperty(req, state);
        });
  d.add("trigger_action",
        WebSocketRequest::kTriggerAction,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleTriggerAction(req, state);
        });
  d.add("highlight",
        WebSocketRequest::kHighlight,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleHighlight(req, state);
        });
  d.add("unhighlight",
        WebSocketRequest::kUnhighlight,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleUnhighlight(req, state);
        });
  d.add("clear_highlights",
        WebSocketRequest::kClearHighlights,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleClearHighlights(req, state);
        });
  d.add("list_selection",
        WebSocketRequest::kListSelection,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleListSelection(req, state);
        });
  d.add("inspect_selection",
        WebSocketRequest::kInspectSelection,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleInspectSelection(req, state);
        });
  d.add("inspect_group",
        WebSocketRequest::kInspectGroup,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleInspectGroup(req, state);
        });
  d.add("deselect",
        WebSocketRequest::kDeselect,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleDeselect(req, state);
        });
  d.add("select_layer",
        WebSocketRequest::kSelectLayer,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSelectLayer(req, state);
        });
  d.add("set_focus_nets",
        WebSocketRequest::kSetFocusNets,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSetFocusNets(req, state);
        });
  d.add("select_fanout_bin",
        WebSocketRequest::kSelectFanoutBin,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSelectFanoutBin(req, state);
        });
  d.add("select_net_length_bin",
        WebSocketRequest::kSelectNetLengthBin,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSelectNetLengthBin(req, state);
        });
  d.add("find",
        WebSocketRequest::kFind,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleFind(req, state);
        });
  d.add("set_route_guides",
        WebSocketRequest::kSetRouteGuides,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSetRouteGuides(req, state);
        });
  d.add("get_3d_data",
        WebSocketRequest::kGet3DData,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleGet3DData(req);
        });
  d.add("context_action",
        WebSocketRequest::kContextAction,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleContextAction(req, state);
        });
}

WebSocketResponse SelectHandler::handleSelect(const WebSocketRequest& req,
                                              SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    consumeStaleSelection(state);
    TileVisibility vis;
    vis.parseFromJson(req.json);
    // Leaflet allows fractional zoom (zoomSnap: 0); accept either int or
    // double on the wire and truncate to selectAt's integer zoom level.
    const auto& zoom_v = req.json.at("zoom");
    const int zoom = zoom_v.is_int64() ? static_cast<int>(zoom_v.get_int64())
                                       : static_cast<int>(zoom_v.as_double());
    auto results
        = gen_->selectAt(static_cast<int>(req.json.at("dbu_x").as_int64()),
                         static_cast<int>(req.json.at("dbu_y").as_int64()),
                         zoom,
                         vis,
                         arrayAsStringSet(req.json.at("visible_layers")));

    const bool add_to_selection = jsonOr(req.json, "add_to_selection", false);
    // Right-click (context menu) select: only replace the selection when the
    // cursor is over a NOT-already-selected object; otherwise keep it.
    const bool context = jsonOr(req.json, "context", false);
    const bool show_connectivity = jsonOr(req.json, "show_connectivity", false);

    // STA's highlight() and getProperties() are not thread-safe;
    // serialize with other STA callers (timing, clock tree, tcl eval).
    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
    const bool use_dbu = jsonOr(req.json, "use_dbu", false);
    ScopedDbuFormat dbu_fmt(gen_->getDb(), use_dbu);

    resp.type = WebSocketResponse::kJson;
    boost::json::object root;
    boost::json::array selected;
    selected.reserve(results.size());
    for (const auto& r : results) {
      boost::json::object item;
      item["name"] = r.name;
      item["type"] = r.type_name;
      item["bbox"] = bboxArray(r.bbox);
      selected.emplace_back(std::move(item));
    }
    root["selected"] = std::move(selected);

    // Pick which result to inspect, cycling through overlapping objects.
    // If the currently inspected object is in the results, select the next one.
    std::vector<gui::Selected> new_selectables;
    auto* registry = gui::DescriptorRegistry::instance();
    gui::Selected inspected_sel;
    if (!results.empty()) {
      int pick = 0;
      if (results.size() > 1) {
        gui::Selected current;
        {
          std::lock_guard<std::mutex> lock(state.selection_mutex);
          current = state.current_inspected;
        }
        if (current) {
          for (int i = 0; i < static_cast<int>(results.size()); ++i) {
            gui::Selected candidate = registry->makeSelected(results[i].object);
            if (candidate == current) {
              pick = (i + 1) % static_cast<int>(results.size());
              break;
            }
          }
        }
      }
      inspected_sel = registry->makeSelected(results[pick].object);
      int hl_group = -1;
      {
        std::lock_guard<std::mutex> lock(state.selection_mutex);
        hl_group = highlightGroupOfLocked(state, inspected_sel);
      }
      // Pass the hit's chiplet transform so the emitted bbox lands in the same
      // world space as results[pick].bbox.
      writeInspectPayload(root,
                          inspected_sel,
                          new_selectables,
                          /*can_navigate_back=*/false,
                          use_dbu,
                          gen_->getLogger(),
                          hl_group,
                          results[pick].world_xfm);
    } else {
      root["can_navigate_back"] = 0;
    }
    {
      std::lock_guard<std::mutex> lock(state.selectables_mutex);
      state.selectables = std::move(new_selectables);
    }
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      state.hover_rects.clear();
      state.timing_rects.clear();
      state.timing_lines.clear();
      state.navigation_history.clear();

      if (add_to_selection) {
        // Shift+click: add to existing selection set if we hit something;
        // clicking empty space preserves the current selection.
        if (inspected_sel) {
          state.selection_itr = state.selection_set.insert(inspected_sel).first;
        }
      } else if (context
                 && (!inspected_sel
                     || state.selection_set.contains(inspected_sel))) {
        // Right-click on empty space or on an already-selected object: keep the
        // current selection so the context menu operates on it.
        if (inspected_sel) {
          state.selection_itr = state.selection_set.find(inspected_sel);
        }
      } else {
        // Normal click, or a context click on a new (unselected) object:
        // replace the selection set.
        state.selection_set.clear();
        if (inspected_sel) {
          state.selection_set.insert(inspected_sel);
        }
        state.selection_itr = state.selection_set.begin();
      }

      if (show_connectivity) {
        // std::set insertion keeps selection_itr valid.
        root["connected_added"]
            = static_cast<int64_t>(addConnectedNets(state.selection_set));
      }

      // Highlight all items in the selection set.
      setSelectionSetHighlights(state);
      // Keep the current inspected object on an empty context click: the menu
      // has to keep describing what the user last inspected.
      if (inspected_sel || !context) {
        runDeselectAction(state.current_inspected, inspected_sel);
        state.current_inspected = inspected_sel;
      }

      root["selection_count"]
          = static_cast<int64_t>(state.selection_set.size());
      root["selection_index"] = static_cast<int64_t>(
          selectionIteratorPosition(state.selection_set, state.selection_itr));
      addSelectionTypeFlags(root, state.selection_set);
    }

    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

// ── Context-menu "Select →" actions ───────────────────────────────────────
//
// Mirror the Qt GUI's LayoutViewer "Select →" context menu, operating on the
// session's current selection set.  Connected-object traversal replicates
// MainWindow::selectHighlightConnected{Insts,Nets,BufferTrees}
// (mainWindow.cpp).

namespace {

// True if `inst` is a buffer cell (per liberty).  When `include_inverters` is
// set, single-input inverters also count, so buffer-tree traversal follows
// inverter-based repeater chains (a superset of the Qt behavior, opt-in).
bool isRepeaterInst(odb::dbInst* inst, sta::dbSta* sta, bool include_inverters)
{
  if (inst == nullptr || sta == nullptr) {
    return false;
  }
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::Cell* cell = network->dbToSta(inst->getMaster());
  if (cell == nullptr) {
    return false;
  }
  sta::LibertyCell* lib_cell = network->libertyCell(cell);
  if (lib_cell == nullptr) {
    return false;
  }
  return lib_cell->isBuffer() || (include_inverters && lib_cell->isInverter());
}

// Walk the buffer tree rooted at `net`, collecting every net and buffer inst
// reachable by passing through buffer (optionally inverter) cells.  Mirrors
// gui::BufferTree::populate without depending on the gui module's private
// BufferTree type.
void collectBufferTree(odb::dbNet* net,
                       sta::dbSta* sta,
                       bool include_inverters,
                       odb::PtrSet<odb::dbNet>& nets,
                       odb::PtrSet<odb::dbInst>& insts)
{
  if (net == nullptr || nets.contains(net)) {
    return;
  }
  nets.insert(net);
  for (auto* iterm : net->getITerms()) {
    odb::dbInst* inst = iterm->getInst();
    if (!isRepeaterInst(inst, sta, include_inverters)) {
      continue;
    }
    insts.insert(inst);
    for (auto* next : inst->getITerms()) {
      if (!next->getSigType().isSupply()) {
        collectBufferTree(next->getNet(), sta, include_inverters, nets, insts);
      }
    }
  }
}

// Collect the objects connected to the current selection, per `action`.
gui::SelectionSet computeConnectedSet(const std::string& action,
                                      const gui::SelectionSet& selection,
                                      const bool output,
                                      const bool input,
                                      const bool include_inverters,
                                      sta::dbSta* sta)
{
  auto* registry = gui::DescriptorRegistry::instance();
  gui::SelectionSet result;

  const bool want_insts = action.find("insts") != std::string::npos;
  const bool want_buffer_trees
      = action.find("buffer_trees") != std::string::npos;
  const bool want_nets
      = !want_buffer_trees && action.find("nets") != std::string::npos;

  for (const auto& sel : selection) {
    if (!sel) {
      continue;
    }
    if (want_insts && sel.isNet()) {
      // "Connected Insts": from selected net(s), select the owning instances
      // of the net's terminals (deduped by the SelectionSet) so they show as
      // bboxes, matching the label.
      auto* net = std::any_cast<odb::dbNet*>(sel.getObject());
      if (net == nullptr) {
        continue;
      }
      for (auto* iterm : net->getITerms()) {
        odb::dbInst* inst = iterm->getInst();
        if (inst != nullptr) {
          result.insert(registry->makeSelected(inst));
        }
      }
    } else if (want_nets && sel.isInst()) {
      // "Output/Input/All Nets": from selected inst(s), select signal nets by
      // pin direction (mirrors MainWindow::selectHighlightConnectedNets).
      auto* inst = std::any_cast<odb::dbInst*>(sel.getObject());
      if (inst == nullptr) {
        continue;
      }
      for (auto* iterm : inst->getITerms()) {
        odb::dbNet* net = iterm->getNet();
        if (net == nullptr || net->getSigType() != odb::dbSigType::SIGNAL) {
          continue;
        }
        const auto dir = iterm->getIoType();
        if (output
            && (dir == odb::dbIoType::OUTPUT || dir == odb::dbIoType::INOUT)) {
          result.insert(registry->makeSelected(net));
        }
        if (input
            && (dir == odb::dbIoType::INPUT || dir == odb::dbIoType::INOUT)) {
          // TODO: the Qt GUI selects DbNetDescriptor::NetWithSink{net, iterm}
          // so the input flightline points at this specific sink pin.  That
          // descriptor lives in a private gui header; until it is exposed the
          // web selects the plain net (all sinks highlighted).
          result.insert(registry->makeSelected(net));
        }
      }
    } else if (want_buffer_trees && sel.isInst()) {
      // "All buffer trees": from selected inst(s), expand the buffer tree(s)
      // reachable through its signal pins.
      auto* inst = std::any_cast<odb::dbInst*>(sel.getObject());
      if (inst == nullptr) {
        continue;
      }
      odb::PtrSet<odb::dbNet> tree_nets;
      odb::PtrSet<odb::dbInst> tree_insts;
      for (auto* iterm : inst->getITerms()) {
        const auto dir = iterm->getIoType();
        if (iterm->getSigType().isSupply()
            || (dir != odb::dbIoType::INPUT && dir != odb::dbIoType::OUTPUT
                && dir != odb::dbIoType::INOUT)) {
          continue;
        }
        odb::dbNet* net = iterm->getNet();
        if (net == nullptr || net->getSigType() != odb::dbSigType::SIGNAL) {
          continue;
        }
        collectBufferTree(net, sta, include_inverters, tree_nets, tree_insts);
      }
      // Select the tree's nets and buffer instances via their already
      // registered dbNet/dbInst descriptors (no gui::BufferTree dependency).
      for (auto* tree_net : tree_nets) {
        result.insert(registry->makeSelected(tree_net));
      }
      for (auto* tree_inst : tree_insts) {
        result.insert(registry->makeSelected(tree_inst));
      }
    }
  }
  return result;
}

}  // namespace

WebSocketResponse SelectHandler::handleContextAction(
    const WebSocketRequest& req,
    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    const std::string action(req.json.at("action").as_string());
    const bool output = jsonOr(req.json, "output", false);
    const bool input = jsonOr(req.json, "input", false);
    const bool include_inverters = jsonOr(req.json, "include_inverters", false);
    const int group = static_cast<int>(jsonOr(req.json, "group", int64_t{0}));
    if (group < 0 || group >= gui::kNumHighlightSet) {
      throw std::runtime_error("invalid highlight group");
    }

    // STA traversal and makeSelected() are not thread-safe; serialize with
    // other STA callers (select, timing, clock tree, tcl eval).
    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
    std::lock_guard<std::mutex> lock(state.selection_mutex);

    // Reset helpers, composed by clear_all.
    auto clearSelection = [&]() {
      state.selection_set.clear();
      state.selection_itr = state.selection_set.end();
      state.current_inspected = gui::Selected();
      state.navigation_history.clear();
      clearSelectionHighlights(state);
    };
    auto clearHighlightGroups = [&]() {
      for (auto& members : state.highlight_groups) {
        members.clear();
      }
      rebuildHighlightGroupShapesLocked(state);
    };
    auto clearFocusNets = [&]() {
      std::lock_guard<std::mutex> flock(state.focus_nets_mutex);
      state.focus_net_ids.clear();
    };
    auto clearRouteGuides = [&]() {
      std::lock_guard<std::mutex> rlock(state.route_guides_mutex);
      state.route_guide_net_ids.clear();
    };

    int connected_count = 0;
    if (action.starts_with("select_") || action.starts_with("highlight_")) {
      // Both branches resolve the same connected set (computeConnectedSet keys
      // off the "insts"/"nets"/"buffer_trees" substring, not the prefix).
      const gui::SelectionSet connected
          = computeConnectedSet(action,
                                state.selection_set,
                                output,
                                input,
                                include_inverters,
                                gen_->getSta());
      if (action.starts_with("select_")) {
        for (const auto& s : connected) {
          if (state.selection_set.insert(s).second) {
            ++connected_count;
          }
        }
        state.selection_itr = state.selection_set.begin();
        setSelectionSetHighlights(state);
      } else {
        // An object lives in at most one group, so a re-highlight moves it.
        for (const auto& s : connected) {
          if (!s) {
            continue;
          }
          ++connected_count;
          removeFromHighlightGroupsLocked(state, s);
          state.highlight_groups[group].insert(s);
        }
        rebuildHighlightGroupShapesLocked(state);
      }
    } else if (action == "clear_highlights") {
      clearHighlightGroups();
    } else if (action == "clear_selections") {
      clearSelection();
    } else if (action == "clear_focus_nets") {
      clearFocusNets();
    } else if (action == "clear_route_guides") {
      clearRouteGuides();
    } else if (action == "clear_all") {
      clearSelection();
      clearHighlightGroups();
      clearFocusNets();
      clearRouteGuides();
    } else {
      throw std::runtime_error("unknown context action: " + action);
    }

    boost::json::object root;
    root["ok"] = true;
    // connected_count == 0 lets the frontend surface "nothing connected —
    // select an instance/net first" (the Qt GUI is silent in this case).
    root["connected_count"] = static_cast<int64_t>(connected_count);
    root["selection_count"] = static_cast<int64_t>(state.selection_set.size());
    addSelectionTypeFlags(root, state.selection_set);
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("context_action error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleInspect(const WebSocketRequest& req,
                                               SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    consumeStaleSelection(state);
    gui::Selected sel;
    {
      const int select_id
          = static_cast<int>(req.json.at("select_id").as_int64());
      if (select_id >= 0) {
        std::lock_guard<std::mutex> lock(state.selectables_mutex);
        if (select_id < static_cast<int>(state.selectables.size())) {
          sel = state.selectables[select_id];
        }
      } else {
        // select_id < 0: re-inspect the currently inspected object
        // (used when toggling display-unit mode without changing selection).
        std::lock_guard<std::mutex> lock(state.selection_mutex);
        sel = state.current_inspected;
      }
    }

    // STA's highlight() and getProperties() are not thread-safe;
    // serialize with other STA callers (timing, clock tree, tcl eval).
    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
    const bool use_dbu = jsonOr(req.json, "use_dbu", false);
    ScopedDbuFormat dbu_fmt(gen_->getDb(), use_dbu);

    bool can_navigate_back = false;
    int sel_count = 0;
    int sel_index = -1;
    int hl_group = -1;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      state.hover_rects.clear();
      state.timing_rects.clear();
      state.timing_lines.clear();
      setSelectionHighlights(state, sel);
      if (sel) {
        if (state.current_inspected && state.current_inspected != sel) {
          state.navigation_history.push_back(state.current_inspected);
        }
        runDeselectAction(state.current_inspected, sel);
        state.current_inspected = sel;
        // Realign the cycling iterator with the linked target so that
        // selection_index reflects the object actually being rendered, and
        // the next Next/Previous starts from this object. If the link goes
        // outside the multi-selection, point the iterator at end() so the
        // index serializes as -1 and the nav UI is suppressed.
        state.selection_itr = state.selection_set.find(sel);
      }
      can_navigate_back = !state.navigation_history.empty();
      sel_count = static_cast<int>(state.selection_set.size());
      sel_index
          = selectionIteratorPosition(state.selection_set, state.selection_itr);
      hl_group = highlightGroupOfLocked(state, sel);
    }

    resp.type = WebSocketResponse::kJson;
    boost::json::object root;
    writeInspectTrailer(root,
                        state,
                        sel,
                        can_navigate_back,
                        use_dbu,
                        gen_->getLogger(),
                        hl_group,
                        sel_count,
                        sel_index);
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleInspectBack(const WebSocketRequest& req,
                                                   SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    consumeStaleSelection(state);
    gui::Selected sel;
    bool can_navigate_back = false;

    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
    const bool use_dbu = jsonOr(req.json, "use_dbu", false);
    ScopedDbuFormat dbu_fmt(gen_->getDb(), use_dbu);
    int sel_count = 0;
    int sel_index = -1;
    int hl_group = -1;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      state.hover_rects.clear();
      state.timing_rects.clear();
      state.timing_lines.clear();

      if (!state.navigation_history.empty()) {
        sel = state.navigation_history.back();
        state.navigation_history.pop_back();
        runDeselectAction(state.current_inspected, sel);
        state.current_inspected = sel;
      } else {
        sel = state.current_inspected;
      }

      setSelectionHighlights(state, sel);
      can_navigate_back = !state.navigation_history.empty();
      sel_count = static_cast<int>(state.selection_set.size());
      sel_index
          = selectionIteratorPosition(state.selection_set, state.selection_itr);
      hl_group = highlightGroupOfLocked(state, sel);
    }

    resp.type = WebSocketResponse::kJson;
    boost::json::object root;
    writeInspectTrailer(root,
                        state,
                        sel,
                        can_navigate_back,
                        use_dbu,
                        gen_->getLogger(),
                        hl_group,
                        sel_count,
                        sel_index);
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

// Cycle to the next/previous item in the multi-selection set.
// Returns the inspect payload for the newly active item without
// changing the highlight shapes (all selected items stay highlighted).
static WebSocketResponse handleSelectionCycle(
    const WebSocketRequest& req,
    SessionState& state,
    const int direction,
    std::shared_ptr<TclEvaluator>& tcl_eval,
    odb::dbDatabase* db,
    utl::Logger* logger)
{
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    consumeStaleSelection(state);
    gui::Selected sel;

    std::lock_guard<std::mutex> sta_lock(tcl_eval->mutex);
    const bool use_dbu = jsonOr(req.json, "use_dbu", false);
    ScopedDbuFormat dbu_fmt(db, use_dbu);
    int sel_count = 0;
    int sel_index = -1;
    int hl_group = -1;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      sel_count = static_cast<int>(state.selection_set.size());
      if (sel_count > 0) {
        if (direction > 0) {
          ++state.selection_itr;
          if (state.selection_itr == state.selection_set.end()) {
            state.selection_itr = state.selection_set.begin();
          }
        } else {
          if (state.selection_itr == state.selection_set.begin()) {
            state.selection_itr = state.selection_set.end();
          }
          --state.selection_itr;
        }
        sel = *state.selection_itr;
        runDeselectAction(state.current_inspected, sel);
        state.current_inspected = sel;
        state.hover_rects.clear();
        state.timing_rects.clear();
        state.timing_lines.clear();
        state.navigation_history.clear();
        // Restore selection-set highlights (handleInspect may have
        // replaced them with a single linked object's shapes).
        setSelectionSetHighlights(state);
      }
      sel_index
          = selectionIteratorPosition(state.selection_set, state.selection_itr);
      hl_group = highlightGroupOfLocked(state, sel);
    }

    resp.type = WebSocketResponse::kJson;
    boost::json::object root;
    writeInspectTrailer(root,
                        state,
                        sel,
                        /*can_navigate_back=*/false,
                        use_dbu,
                        logger,
                        hl_group,
                        sel_count,
                        sel_index);
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleSelectNext(const WebSocketRequest& req,
                                                  SessionState& state)
{
  return handleSelectionCycle(
      req, state, +1, tcl_eval_, gen_->getDb(), gen_->getLogger());
}

WebSocketResponse SelectHandler::handleSelectPrev(const WebSocketRequest& req,
                                                  SessionState& state)
{
  return handleSelectionCycle(
      req, state, -1, tcl_eval_, gen_->getDb(), gen_->getLogger());
}

// Commit an edit to a property of the currently inspected object via the
// descriptor's Editor callback (Qt GUI parity: EditorItemDelegate::
// setModelData).  List editors commit by option index so the exact
// std::any stored in the option reaches the callback; scalar editors
// marshal the JSON value per the Qt delegate's rules (numbers always
// arrive as double).  On success the full inspect payload is rebuilt —
// the edit may have moved or renamed the object — and a {"type":"refresh"}
// push is broadcast to every session once the STA lock is released.
// On rejection the payload carries ok:0 plus the reason; the client
// re-renders from it (visible revert, unlike Qt's silent one).
WebSocketResponse SelectHandler::handleSetProperty(const WebSocketRequest& req,
                                                   SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  bool accepted = false;
  // Built under sta_lock (getBounds reads odb); broadcast after the lock.
  std::string broadcast_payload;
  try {
    const std::string name(req.json.at("name").as_string());

    // STA's getProperties() and the editor callbacks (which mutate the
    // database) are not thread-safe; serialize with other STA callers.
    // Also blocks edits while a Tcl command runs (Qt's read-only guard).
    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
    const bool use_dbu = jsonOr(req.json, "use_dbu", false);
    ScopedDbuFormat dbu_fmt(gen_->getDb(), use_dbu);

    std::string error;
    if (consumeStaleSelection(state)) {
      error = "selection invalidated by a design change; reselect and retry";
    }
    gui::Selected sel;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      sel = state.current_inspected;
    }

    if (!error.empty()) {
      // fall through with the staleness error
    } else if (!sel) {
      error = "nothing is inspected";
    } else {
      const gui::Descriptor::Editors editors = sel.getEditors();
      const auto editor_it = editors.find(name);
      if (editor_it == editors.end()) {
        error = "property is not editable: " + name;
      } else {
        std::any callback_value;
        bool value_valid = false;
        if (const auto* idx_v = req.json.if_contains("option_index")) {
          const auto idx = idx_v->as_int64();
          const auto& options = editor_it->second.options;
          if (idx < 0 || idx >= static_cast<int64_t>(options.size())) {
            error = "invalid option index";
          } else if (jsonOr(req.json, "option_name", std::string())
                     != options[idx].name) {
            // The option list was re-fetched at commit time and no longer
            // matches what the client rendered (e.g. the DB changed).
            error = "options changed; reselect the object and retry";
          } else {
            callback_value = options[idx].value;
            value_valid = true;
          }
        } else {
          const auto& value = req.json.at("value");
          if (value.is_bool()) {
            callback_value = value.get_bool();
            value_valid = true;
          } else if (value.is_string()) {
            callback_value = std::string(value.as_string());
            value_valid = true;
          } else if (value.is_int64() || value.is_uint64()
                     || value.is_double()) {
            callback_value = value.to_number<double>();
            value_valid = true;
          } else {
            error = "unsupported value type";
          }
        }
        if (value_valid) {
          try {
            accepted = editor_it->second.callback(callback_value);
            if (!accepted) {
              error = "value rejected for property: " + name;
            }
          } catch (const std::exception& e) {
            // utl::Logger::error throws std::runtime_error; a bad_any_cast
            // would be an editor-contract bug.  Either way: reject with
            // the reason, never let it escape to the session thread.
            accepted = false;
            error = e.what();
          }
        }
      }
    }

    bool can_navigate_back = false;
    int sel_count = 0;
    int sel_index = -1;
    int hl_group = -1;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      if (accepted) {
        // Geometry may have changed (e.g. X/Y edit); rebuild highlights.
        if (state.selection_set.find(sel) != state.selection_set.end()) {
          setSelectionSetHighlights(state);
        } else {
          setSelectionHighlights(state, sel);
        }
        // Group shapes derive from geometry too.
        rebuildHighlightGroupShapesLocked(state);
      }
      can_navigate_back = !state.navigation_history.empty();
      sel_count = static_cast<int>(state.selection_set.size());
      sel_index
          = selectionIteratorPosition(state.selection_set, state.selection_itr);
      hl_group = highlightGroupOfLocked(state, sel);
    }

    resp.type = WebSocketResponse::kJson;
    boost::json::object root;
    writeInspectTrailer(root,
                        state,
                        sel,
                        can_navigate_back,
                        use_dbu,
                        gen_->getLogger(),
                        hl_group,
                        sel_count,
                        sel_index);
    // After the trailer so the handler error wins over writeInspectPayload's
    // "invalid select_id" default when nothing is inspected.
    root["ok"] = accepted ? 1 : 0;
    if (!accepted) {
      root["error"] = error;
    }
    if (accepted) {
      broadcast_payload = refreshBroadcastPayload(*gen_);
    }
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  if (accepted && broadcast_fn_) {
    broadcast_fn_(broadcast_payload);
  }
  return resp;
}

// Run a descriptor action (Qt GUI parity: Inspector::handleAction) on the
// currently inspected object.  The action is re-resolved by name at
// trigger time — never by index — and blocklisted/reserved names are
// refused.  The callback's returned Selected becomes the new inspected
// object (empty → cleared inspector).  When the action destroys objects
// (e.g. Delete) the session's own odb destroy callbacks raise
// selection_stale; the whole selection state is rebuilt from the returned
// object alone since history/selection-set entries may now dangle.
WebSocketResponse SelectHandler::handleTriggerAction(
    const WebSocketRequest& req,
    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  bool executed = false;
  // Built under sta_lock (getBounds reads odb); broadcast after the lock.
  std::string broadcast_payload;
  try {
    const std::string name(req.json.at("name").as_string());

    // STA descriptor calls and the action callbacks (which may mutate the
    // database) are not thread-safe; serialize with other STA callers.
    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
    const bool use_dbu = jsonOr(req.json, "use_dbu", false);
    ScopedDbuFormat dbu_fmt(gen_->getDb(), use_dbu);

    consumeStaleSelection(state);
    gui::Selected sel;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      sel = state.current_inspected;
    }

    std::string error;
    gui::Selected next;
    if (!sel) {
      error = "nothing is inspected";
    } else if (name == gui::Descriptor::kDeselectAction
               || kSuppressedActions.find(name) != kSuppressedActions.end()) {
      error = "action is not available: " + name;
    } else {
      const gui::Descriptor::Action* action = nullptr;
      const auto actions = sel.getDescriptorActions();
      for (const auto& candidate : actions) {
        if (candidate.name == name) {
          action = &candidate;
          break;
        }
      }
      if (action == nullptr) {
        error = "action no longer available: " + name;
      } else {
        try {
          next = action->callback();
          executed = true;
        } catch (const std::exception& e) {
          error = e.what();
        }
      }
    }

    // A destroy performed by the action raised our own staleness flag;
    // consume it and note that the previous object is gone.
    const bool deleted = state.selection_stale.exchange(false);

    bool can_navigate_back = false;
    int sel_count = 0;
    int sel_index = -1;
    int hl_group = -1;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      if (deleted) {
        // History and selection-set entries may reference destroyed
        // objects; rebuild from the action's returned object alone.
        state.navigation_history.clear();
        state.selection_set.clear();
        if (next) {
          state.selection_set.insert(next);
        }
        state.selection_itr = state.selection_set.begin();
        state.hover_rects.clear();
        state.current_inspected = next;
        setSelectionHighlights(state, next);
        // Highlight-group members may also reference destroyed objects.
        for (auto& members : state.highlight_groups) {
          members.clear();
        }
        state.highlight_group_rects.clear();
        state.highlight_group_polys.clear();
        state.highlight_group_lines.clear();
      } else if (executed && next != sel) {
        runDeselectAction(sel, next);
        if (next) {
          state.navigation_history.push_back(sel);
        }
        state.current_inspected = next;
        state.selection_itr = state.selection_set.find(next);
        state.hover_rects.clear();
        setSelectionHighlights(state, next);
      } else if (executed) {
        next = sel;  // action kept the selection (e.g. a toggle)
      }
      can_navigate_back = !state.navigation_history.empty();
      sel_count = static_cast<int>(state.selection_set.size());
      sel_index
          = selectionIteratorPosition(state.selection_set, state.selection_itr);
      hl_group
          = highlightGroupOfLocked(state, (deleted || executed) ? next : sel);
    }

    resp.type = WebSocketResponse::kJson;
    boost::json::object root;
    // After a destroy, `sel` dangles — the payload must come from `next`.
    const gui::Selected& payload_sel = (deleted || executed) ? next : sel;
    writeInspectTrailer(root,
                        state,
                        payload_sel,
                        can_navigate_back,
                        use_dbu,
                        gen_->getLogger(),
                        hl_group,
                        sel_count,
                        sel_index);
    if (executed && !next) {
      // The action deselected (e.g. Delete): an empty payload with no
      // "error" clears the client inspector.
      root.erase("error");
    }
    root["ok"] = executed ? 1 : 0;
    if (!executed) {
      root["error"] = error;
    }
    root["deleted"] = deleted ? 1 : 0;
    if (executed) {
      broadcast_payload = refreshBroadcastPayload(*gen_);
    }
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  if (executed && broadcast_fn_) {
    broadcast_fn_(broadcast_payload);
  }
  return resp;
}

// Put the currently inspected object into color group `group` (0-15),
// removing it from any other group first — an object lives in at most
// one group.  Mirrors the Qt GUI's MainWindow::updateHighlightedSet
// semantics (the Qt add path can leave an object in two groups; the web
// always enforces uniqueness).  Responds with the refreshed inspect
// payload so the client updates the group badge in place.
WebSocketResponse SelectHandler::handleHighlight(const WebSocketRequest& req,
                                                 SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    const int64_t group = req.json.at("group").as_int64();

    // sel.highlight() (shape collection) runs descriptor code; serialize
    // with other STA callers.
    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
    const bool use_dbu = jsonOr(req.json, "use_dbu", false);
    ScopedDbuFormat dbu_fmt(gen_->getDb(), use_dbu);

    std::string error;
    if (consumeStaleSelection(state)) {
      error = "selection invalidated by a design change; reselect and retry";
    }
    gui::Selected sel;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      sel = state.current_inspected;
    }

    bool ok = false;
    if (!error.empty()) {
      // fall through with the staleness error
    } else if (group < 0 || group >= gui::kNumHighlightSet) {
      error = "invalid highlight group";
    } else if (!sel) {
      error = "nothing is inspected";
    } else {
      ok = true;
    }

    bool can_navigate_back = false;
    int sel_count = 0;
    int sel_index = -1;
    int hl_group = -1;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      if (ok) {
        removeFromHighlightGroupsLocked(state, sel);
        state.highlight_groups[group].insert(sel);
        rebuildHighlightGroupShapesLocked(state);
      }
      can_navigate_back = !state.navigation_history.empty();
      sel_count = static_cast<int>(state.selection_set.size());
      sel_index
          = selectionIteratorPosition(state.selection_set, state.selection_itr);
      hl_group = highlightGroupOfLocked(state, sel);
    }

    resp.type = WebSocketResponse::kJson;
    boost::json::object root;
    writeInspectTrailer(root,
                        state,
                        sel,
                        can_navigate_back,
                        use_dbu,
                        gen_->getLogger(),
                        hl_group,
                        sel_count,
                        sel_index);
    root["ok"] = ok ? 1 : 0;
    if (!ok) {
      root["error"] = error;
    }
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

// Remove the currently inspected object from whatever highlight group
// holds it.  Mirrors Qt's MainWindow::removeHighlighted.
WebSocketResponse SelectHandler::handleUnhighlight(const WebSocketRequest& req,
                                                   SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
    const bool use_dbu = jsonOr(req.json, "use_dbu", false);
    ScopedDbuFormat dbu_fmt(gen_->getDb(), use_dbu);

    std::string error;
    if (consumeStaleSelection(state)) {
      error = "selection invalidated by a design change; reselect and retry";
    }
    gui::Selected sel;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      sel = state.current_inspected;
    }

    bool ok = false;
    if (error.empty()) {
      if (!sel) {
        error = "nothing is inspected";
      } else {
        ok = true;
      }
    }

    bool can_navigate_back = false;
    int sel_count = 0;
    int sel_index = -1;
    int hl_group = -1;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      if (ok && removeFromHighlightGroupsLocked(state, sel)) {
        rebuildHighlightGroupShapesLocked(state);
      }
      can_navigate_back = !state.navigation_history.empty();
      sel_count = static_cast<int>(state.selection_set.size());
      sel_index
          = selectionIteratorPosition(state.selection_set, state.selection_itr);
      // On success the object left every group (-1); on the error path it
      // stays put, so report its real group instead of clearing the badge.
      hl_group = ok ? -1 : highlightGroupOfLocked(state, sel);
    }

    resp.type = WebSocketResponse::kJson;
    boost::json::object root;
    writeInspectTrailer(root,
                        state,
                        sel,
                        can_navigate_back,
                        use_dbu,
                        gen_->getLogger(),
                        hl_group,
                        sel_count,
                        sel_index);
    root["ok"] = ok ? 1 : 0;
    if (!ok) {
      root["error"] = error;
    }
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

// Clear one highlight group (0-15) or all of them (group = -1, the
// default — Qt's layout "Clear → Highlights" semantics).
WebSocketResponse SelectHandler::handleClearHighlights(
    const WebSocketRequest& req,
    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    const auto group = jsonOr<int64_t>(req.json, "group", -1);

    // Rebuilding the remaining groups' shapes runs descriptor code.
    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
    consumeStaleSelection(state);

    resp.type = WebSocketResponse::kJson;
    boost::json::object root;
    if (group < -1 || group >= gui::kNumHighlightSet) {
      root["ok"] = 0;
      root["error"] = "invalid highlight group";
      writePayload(resp, root);
      return resp;
    }

    int64_t cleared = 0;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      if (group < 0) {
        for (auto& members : state.highlight_groups) {
          cleared += static_cast<int64_t>(members.size());
          members.clear();
        }
      } else {
        cleared = static_cast<int64_t>(state.highlight_groups[group].size());
        state.highlight_groups[group].clear();
      }
      rebuildHighlightGroupShapesLocked(state);
    }

    root["ok"] = 1;
    root["cleared"] = cleared;
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

// One selection-browser row: name, type, and (when available) bbox so the
// client can zoom without a round-trip.  Runs descriptor code — STA lock.
static boost::json::object browserRow(const gui::Selected& sel)
{
  boost::json::object row;
  row["name"] = sel.getName();
  row["type"] = sel.getTypeName();
  odb::Rect bbox;
  if (sel.getBBox(bbox)) {
    row["bbox"] = bboxArray(bbox);
  }
  return row;
}

// List the selection set and the 16 highlight groups for the selection
// browser (Qt GUI parity: selectHighlightWindow's two tabs).  Read-only:
// does NOT touch state.selectables, so inspector link ids stay valid.
// Each set is capped so a huge selection cannot hang the panel (the Qt
// dock renders everything and freezes).
WebSocketResponse SelectHandler::handleListSelection(
    const WebSocketRequest& req,
    SessionState& state)
{
  constexpr size_t kMaxBrowserRows = 1000;
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    // getName/getTypeName run descriptor (STA-touching) code.
    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
    const bool use_dbu = jsonOr(req.json, "use_dbu", false);
    ScopedDbuFormat dbu_fmt(gen_->getDb(), use_dbu);
    consumeStaleSelection(state);

    bool truncated = false;
    boost::json::object root;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);

      boost::json::array selection;
      for (const auto& sel : state.selection_set) {
        if (!sel) {
          continue;
        }
        if (selection.size() >= kMaxBrowserRows) {
          truncated = true;
          break;
        }
        selection.emplace_back(browserRow(sel));
      }
      root["selection"] = std::move(selection);

      boost::json::array groups;
      for (int group = 0; group < gui::kNumHighlightSet; ++group) {
        boost::json::array members;
        for (const auto& sel : state.highlight_groups[group]) {
          if (!sel) {
            continue;
          }
          if (members.size() >= kMaxBrowserRows) {
            truncated = true;
            break;
          }
          members.emplace_back(browserRow(sel));
        }
        groups.emplace_back(std::move(members));
      }
      root["groups"] = std::move(groups);
    }

    root["ok"] = 1;
    root["truncated"] = truncated;
    resp.type = WebSocketResponse::kJson;
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

// Shared body of inspect_selection / inspect_group: make the `index`-th
// member of `browsing_group` (-1 = the selection set, 0-15 = a highlight
// group) the inspected object and return the full inspect payload.  Rows
// resolve by stable index into the ordered set — never through
// state.selectables, which belongs to the last inspect-style response.
static WebSocketResponse inspectBrowserRow(const WebSocketRequest& req,
                                           SessionState& state,
                                           int browsing_group,
                                           std::shared_ptr<TclEvaluator>& tcl,
                                           odb::dbDatabase* db,
                                           utl::Logger* logger)
{
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    const int64_t index = req.json.at("index").as_int64();

    std::lock_guard<std::mutex> sta_lock(tcl->mutex);
    const bool use_dbu = jsonOr(req.json, "use_dbu", false);
    ScopedDbuFormat dbu_fmt(db, use_dbu);
    consumeStaleSelection(state);

    gui::Selected sel;
    bool can_navigate_back = false;
    int sel_count = 0;
    int sel_index = -1;
    int hl_group = -1;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      const gui::SelectionSet& set
          = browsing_group < 0 ? state.selection_set
                               : state.highlight_groups[browsing_group];
      if (index < 0 || index >= static_cast<int64_t>(set.size())) {
        resp.type = WebSocketResponse::kJson;
        boost::json::object root;
        root["ok"] = 0;
        root["error"] = "invalid row index";
        writePayload(resp, root);
        return resp;
      }
      sel = *std::next(set.begin(), index);

      state.hover_rects.clear();
      state.timing_rects.clear();
      state.timing_lines.clear();
      if (state.current_inspected && state.current_inspected != sel) {
        state.navigation_history.push_back(state.current_inspected);
      }
      runDeselectAction(state.current_inspected, sel);
      state.current_inspected = sel;
      state.selection_itr = state.selection_set.find(sel);
      if (browsing_group < 0) {
        // Selection rows behave like Next/Previous: keep every selected
        // object highlighted.
        setSelectionSetHighlights(state);
      } else {
        // Group rows behave like a link inspect: highlight the object
        // itself (its group color is already visible in the overlay).
        setSelectionHighlights(state, sel);
      }
      can_navigate_back = !state.navigation_history.empty();
      sel_count = static_cast<int>(state.selection_set.size());
      sel_index
          = selectionIteratorPosition(state.selection_set, state.selection_itr);
      hl_group = highlightGroupOfLocked(state, sel);
    }

    resp.type = WebSocketResponse::kJson;
    boost::json::object root;
    root["ok"] = 1;
    writeInspectTrailer(root,
                        state,
                        sel,
                        can_navigate_back,
                        use_dbu,
                        logger,
                        hl_group,
                        sel_count,
                        sel_index);
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleInspectSelection(
    const WebSocketRequest& req,
    SessionState& state)
{
  return inspectBrowserRow(req,
                           state,
                           /*browsing_group=*/-1,
                           tcl_eval_,
                           gen_->getDb(),
                           gen_->getLogger());
}

WebSocketResponse SelectHandler::handleInspectGroup(const WebSocketRequest& req,
                                                    SessionState& state)
{
  const int64_t group = req.json.at("group").as_int64();
  if (group < 0 || group >= gui::kNumHighlightSet) {
    WebSocketResponse resp;
    resp.id = req.id;
    resp.type = WebSocketResponse::kJson;
    boost::json::object root;
    root["ok"] = 0;
    root["error"] = "invalid highlight group";
    writePayload(resp, root);
    return resp;
  }
  return inspectBrowserRow(req,
                           state,
                           static_cast<int>(group),
                           tcl_eval_,
                           gen_->getDb(),
                           gen_->getLogger());
}

// Remove the `index`-th selection-set member (Qt GUI parity:
// SelectHighlightWindow's "De-Select" → MainWindow::removeFromSelected).
// The inspected object is kept even when it is the removed one, matching
// Qt (the inspector keeps showing it until the next selection).
WebSocketResponse SelectHandler::handleDeselect(const WebSocketRequest& req,
                                                SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    const int64_t index = req.json.at("index").as_int64();

    // collectMultiHighlightShapes runs descriptor code — STA lock.
    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
    consumeStaleSelection(state);

    resp.type = WebSocketResponse::kJson;
    boost::json::object root;
    int sel_count = 0;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      if (index < 0
          || index >= static_cast<int64_t>(state.selection_set.size())) {
        root["ok"] = 0;
        root["error"] = "invalid row index";
        writePayload(resp, root);
        return resp;
      }
      state.selection_set.erase(std::next(state.selection_set.begin(), index));
      // The erased element may have been the cycling position.
      state.selection_itr = state.selection_set.begin();
      setSelectionSetHighlights(state);
      sel_count = static_cast<int>(state.selection_set.size());
    }

    root["ok"] = 1;
    root["selection_count"] = static_cast<int64_t>(sel_count);
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

// Select a tech layer by name, as clicking a layer row in the Qt GUI's
// Display Control does (DisplayControls::displayItemSelected emits
// `selected(makeSelected(tech_layer))`).  The layer becomes the inspected
// object so the Inspector panel shows its properties.
//
// A dbTechLayer carries no geometry, so this contributes no highlight shapes;
// collectHighlightShapes still runs to clear whatever the previous selection
// left behind, matching the "replace the selection" semantics of a plain
// (non-shift) click.
WebSocketResponse SelectHandler::handleSelectLayer(const WebSocketRequest& req,
                                                   SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;

  try {
    const std::string layer_name
        = std::string(req.json.at("layer").as_string());

    // Multi-die designs give each chip its own dbTech, so the same layer name
    // can exist in several of them.  The frontend sends the chiplet path its
    // layer row belongs to; single-chip designs omit it and get the design's
    // only tech.
    std::string chiplet_path;
    if (const auto* v = req.json.if_contains("chiplet"); v && v->is_string()) {
      chiplet_path = std::string(v->as_string());
    }

    odb::dbTech* tech = nullptr;
    if (!chiplet_path.empty()) {
      for (const ChipletNode& node : gen_->chiplets()) {
        if (node.path == chiplet_path && node.chip != nullptr) {
          tech = node.chip->getTech();
          break;
        }
      }
    }
    if (tech == nullptr) {
      tech = gen_->getTech();
    }
    if (tech == nullptr) {
      throw std::runtime_error("No tech loaded");
    }

    odb::dbTechLayer* layer = tech->findLayer(layer_name.c_str());
    if (layer == nullptr) {
      throw std::runtime_error("Layer not found: " + layer_name);
    }

    gui::Selected sel
        = gui::DescriptorRegistry::instance()->makeSelected(layer);

    // STA's getProperties() is not thread-safe; serialize with the other
    // STA callers (timing, clock tree, tcl eval).
    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
    const bool use_dbu = jsonOr(req.json, "use_dbu", false);
    ScopedDbuFormat dbu_fmt(gen_->getDb(), use_dbu);

    int sel_count = 0;
    int sel_index = -1;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      state.hover_rects.clear();
      state.timing_rects.clear();
      state.timing_lines.clear();
      state.navigation_history.clear();
      state.selection_set.clear();
      if (sel) {
        state.selection_set.insert(sel);
      }
      state.selection_itr = state.selection_set.begin();
      // Through the helper, not collectHighlightShapes directly: the highlight
      // vectors and the source tag are one invariant, and the tag is what lets
      // a "Flywires only" flip re-derive from the right selection.
      setSelectionSetHighlights(state);
      state.current_inspected = sel;
      sel_count = static_cast<int>(state.selection_set.size());
      sel_index
          = selectionIteratorPosition(state.selection_set, state.selection_itr);
    }

    boost::json::object root;
    std::vector<gui::Selected> new_selectables;
    writeInspectPayload(root,
                        sel,
                        new_selectables,
                        /*can_navigate_back=*/false,
                        use_dbu,
                        gen_->getLogger());
    root["selection_count"] = static_cast<int64_t>(sel_count);
    root["selection_index"] = static_cast<int64_t>(sel_index);
    {
      std::lock_guard<std::mutex> lock(state.selectables_mutex);
      state.selectables = std::move(new_selectables);
    }

    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleHover(const WebSocketRequest& req,
                                             SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    consumeStaleSelection(state);
    int count = 0;
    std::vector<odb::Rect> hover_rects;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      state.hover_rects.clear();

      const int select_id
          = static_cast<int>(req.json.at("select_id").as_int64());
      if (select_id >= 0) {
        gui::Selected sel;
        {
          std::lock_guard<std::mutex> slock(state.selectables_mutex);
          if (select_id < static_cast<int>(state.selectables.size())) {
            sel = state.selectables[select_id];
          }
        }
        if (sel) {
          ShapeCollector collector;
          sel.highlight(collector);
          if (!collector.rects.empty()) {
            state.hover_rects = std::move(collector.rects);
          } else {
            odb::Rect bbox;
            if (sel.getBBox(bbox)) {
              state.hover_rects.push_back(bbox);
            }
          }
          count = static_cast<int>(state.hover_rects.size());
          hover_rects = state.hover_rects;
        }
      }
      // select_id < 0 just clears hover_rects (mouseleave)
    }

    resp.type = WebSocketResponse::kJson;
    boost::json::object root;
    root["ok"] = 1;
    root["count"] = count;
    boost::json::array rects;
    rects.reserve(hover_rects.size());
    for (const auto& rect : hover_rects) {
      rects.emplace_back(bboxArray(rect));
    }
    root["rects"] = std::move(rects);
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleSetFocusNets(const WebSocketRequest& req,
                                                    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    const std::string action = std::string(req.json.at("action").as_string());
    const std::string net_name
        = std::string(req.json.at("net_name").as_string());
    std::lock_guard<std::mutex> lock(state.focus_nets_mutex);
    if (action == "clear") {
      state.focus_net_ids.clear();
    } else {
      odb::dbBlock* block = gen_->getBlock();
      odb::dbNet* net = block ? block->findNet(net_name.c_str()) : nullptr;
      if (net) {
        if (action == "add") {
          state.focus_net_ids.insert(net->getId());
        } else if (action == "remove") {
          state.focus_net_ids.erase(net->getId());
        }
      }
    }
    boost::json::object root;
    root["ok"] = 1;
    root["count"] = static_cast<int>(state.focus_net_ids.size());
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleSelectFanoutBin(
    const WebSocketRequest& req,
    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    consumeStaleSelection(state);
    const int lower = static_cast<int>(req.json.at("lower").as_int64());
    const int upper = static_cast<int>(req.json.at("upper").as_int64());

    odb::dbBlock* block = gen_->getBlock();
    if (!block) {
      throw std::runtime_error("no design loaded");
    }

    // odb is not thread-safe against design-mutating Tcl commands, and the
    // descriptor/getProperties work below is not thread-safe against STA;
    // serialize the entire net walk and inspection with the shared STA lock.
    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
    const bool use_dbu = jsonOr(req.json, "use_dbu", false);
    ScopedDbuFormat dbu_fmt(gen_->getDb(), use_dbu);

    std::vector<odb::dbNet*> matched;
    for (odb::dbNet* net : block->getNets()) {
      if (net->getSigType().isSupply()) {
        continue;
      }
      const int term_count = static_cast<int>(net->getITermCount())
                             + static_cast<int>(net->getBTermCount());
      const int fanout = std::max(0, term_count - 1);
      if (fanout >= lower && fanout < upper) {
        matched.push_back(net);
      }
    }

    boost::json::object root;
    selectMatchedNets(matched, state, root, use_dbu);
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

void SelectHandler::selectMatchedNets(std::vector<odb::dbNet*>& matched,
                                      SessionState& state,
                                      boost::json::object& root,
                                      const bool use_dbu)
{
  root["count"] = static_cast<int>(matched.size());

  // Selecting/highlighting every net in a densely populated bin (tens of
  // thousands of nets in a large design) is prohibitively expensive and can
  // hang or exhaust memory. Cap the selection so navigation stays usable
  // while reporting the true bin count above.
  constexpr size_t kMaxNetSelection = 1000;
  const bool truncated = matched.size() > kMaxNetSelection;
  if (truncated) {
    matched.resize(kMaxNetSelection);
  }
  root["truncated"] = truncated;
  root["selection_limit"] = static_cast<int>(kMaxNetSelection);

  if (matched.empty()) {
    return;
  }

  auto* registry = gui::DescriptorRegistry::instance();
  gui::SelectionSet new_selection;
  for (auto* n : matched) {
    new_selection.insert(registry->makeSelected(n));
  }
  gui::Selected first = registry->makeSelected(matched.front());

  int hl_group = -1;
  {
    std::lock_guard<std::mutex> lock(state.selection_mutex);
    hl_group = highlightGroupOfLocked(state, first);
  }
  std::vector<gui::Selected> new_selectables;
  writeInspectPayload(root,
                      first,
                      new_selectables,
                      /*can_navigate_back=*/false,
                      use_dbu,
                      gen_->getLogger(),
                      hl_group);
  {
    std::lock_guard<std::mutex> lock(state.selectables_mutex);
    state.selectables = std::move(new_selectables);
  }
  {
    std::lock_guard<std::mutex> lock(state.selection_mutex);
    state.hover_rects.clear();
    state.timing_rects.clear();
    state.timing_lines.clear();
    state.navigation_history.clear();

    state.selection_set = std::move(new_selection);
    state.selection_itr = state.selection_set.find(first);
    if (state.selection_itr == state.selection_set.end()) {
      state.selection_itr = state.selection_set.begin();
    }
    // Highlight every selected net in the layout.
    setSelectionSetHighlights(state);
    runDeselectAction(state.current_inspected, first);
    state.current_inspected = first;

    root["selection_count"] = static_cast<int64_t>(state.selection_set.size());
    root["selection_index"] = static_cast<int64_t>(
        selectionIteratorPosition(state.selection_set, state.selection_itr));
  }
}

WebSocketResponse SelectHandler::handleSelectNetLengthBin(
    const WebSocketRequest& req,
    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    consumeStaleSelection(state);
    // Bin edges arrive in the histogram's display units (µm or DBU); convert
    // to DBU to compare against netHpwlDbu().
    const double lower = jsonToDouble(req.json.at("lower"));
    const double upper = jsonToDouble(req.json.at("upper"));

    odb::dbBlock* block = gen_->getBlock();
    if (!block) {
      throw std::runtime_error("no design loaded");
    }

    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
    const bool use_dbu = jsonOr(req.json, "use_dbu", false);
    ScopedDbuFormat dbu_fmt(gen_->getDb(), use_dbu);

    const double dbu_per_micron = std::max(1, block->getDbUnitsPerMicron());
    const double to_dbu = use_dbu ? 1.0 : dbu_per_micron;
    const double lower_dbu = lower * to_dbu;
    const double upper_dbu = upper * to_dbu;

    std::vector<odb::dbNet*> matched;
    for (odb::dbNet* net : block->getNets()) {
      if (net->getSigType().isSupply()) {
        continue;
      }
      const double hpwl = netHpwlDbu(net);
      if (hpwl >= lower_dbu && hpwl < upper_dbu) {
        matched.push_back(net);
      }
    }

    boost::json::object root;
    selectMatchedNets(matched, state, root, use_dbu);
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

// Find objects by name/glob (mirrors the Qt FindObjectDialog + Gui::select):
// obj_type in {inst, net, port}; pattern is a Unix glob (*, ?, []) or an exact
// name; selects all matches and returns their union bbox for auto-zoom.
WebSocketResponse SelectHandler::handleFind(const WebSocketRequest& req,
                                            SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    const std::string obj_type(req.json.at("obj_type").as_string());
    const std::string pattern(req.json.at("pattern").as_string());
    const bool match_case = jsonOr(req.json, "match_case", false);
    const bool use_dbu = jsonOr(req.json, "use_dbu", false);

    odb::dbBlock* block = gen_->getBlock();
    if (block == nullptr) {
      throw std::runtime_error("no design loaded");
    }

    // STA highlight()/getProperties() and makeSelected() are not thread-safe.
    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
    ScopedDbuFormat dbu_fmt(gen_->getDb(), use_dbu);

    const int fn_flags = match_case ? 0 : FNM_CASEFOLD;
    auto matches = [&](const char* name) {
      return name != nullptr && fnmatch(pattern.c_str(), name, fn_flags) == 0;
    };

    auto* registry = gui::DescriptorRegistry::instance();
    gui::SelectionSet found;
    constexpr size_t kMaxFindSelection = 1000;
    int total = 0;
    auto collect = [&](auto&& range) {
      for (auto* obj : range) {
        if (matches(obj->getConstName())) {
          ++total;
          if (found.size() < kMaxFindSelection) {
            found.insert(registry->makeSelected(obj));
          }
        }
      }
    };
    if (obj_type == "inst") {
      collect(block->getInsts());
    } else if (obj_type == "net") {
      collect(block->getNets());
    } else if (obj_type == "port") {
      collect(block->getBTerms());
    } else {
      throw std::runtime_error("unknown obj_type: " + obj_type);
    }

    boost::json::object root;
    root["count"] = static_cast<int64_t>(total);
    root["truncated"] = total > static_cast<int>(kMaxFindSelection);

    if (!found.empty()) {
      // Union bbox of all matched objects (for the frontend auto-zoom).
      odb::Rect uni;
      uni.mergeInit();
      for (const auto& sel : found) {
        odb::Rect b;
        if (sel && sel.getBBox(b)) {
          uni.merge(b);
        }
      }
      if (!uni.isInverted()) {
        boost::json::array bbox;
        bbox.emplace_back(uni.xMin());
        bbox.emplace_back(uni.yMin());
        bbox.emplace_back(uni.xMax());
        bbox.emplace_back(uni.yMax());
        root["bbox"] = std::move(bbox);
      }

      const gui::Selected first = *found.begin();
      std::vector<gui::Selected> new_selectables;
      int hl_group = -1;
      {
        std::lock_guard<std::mutex> lock(state.selection_mutex);
        hl_group = highlightGroupOfLocked(state, first);
      }
      writeInspectPayload(root,
                          first,
                          new_selectables,
                          /*can_navigate_back=*/false,
                          use_dbu,
                          gen_->getLogger(),
                          hl_group);
      {
        std::lock_guard<std::mutex> lock(state.selectables_mutex);
        state.selectables = std::move(new_selectables);
      }
      {
        std::lock_guard<std::mutex> lock(state.selection_mutex);
        state.hover_rects.clear();
        state.timing_rects.clear();
        state.timing_lines.clear();
        state.navigation_history.clear();
        // Qt parity: Gui::select() hands its matches to
        // MainWindow::addSelected, so a search ADDS to the selection rather
        // than replacing it -- successive searches accumulate, and a
        // selection the user built by hand survives one.  "Clear ->
        // Selections" is how you start over.
        // The cycling iterator is taken from the insert of the FIRST match,
        // so prev/next start from what was just found rather than from the
        // merged set's first element.  Kept from the insert rather than a
        // later find(): `first` is already in the set by then, and re-finding
        // it would only repeat the comparisons the insert has done.
        auto first_itr = state.selection_set.end();
        for (const auto& sel : found) {
          const auto [itr, inserted] = state.selection_set.insert(sel);
          if (first_itr == state.selection_set.end()) {
            first_itr = itr;  // `found` is ordered, so this is `first`
          }
        }
        state.selection_itr = first_itr != state.selection_set.end()
                                  ? first_itr
                                  : state.selection_set.begin();
        setSelectionSetHighlights(state);
        runDeselectAction(state.current_inspected, first);
        state.current_inspected = first;
        root["selection_count"]
            = static_cast<int64_t>(state.selection_set.size());
        root["selection_index"]
            = static_cast<int64_t>(selectionIteratorPosition(
                state.selection_set, state.selection_itr));
      }
    } else {
      root["selection_count"] = static_cast<int64_t>(0);
    }

    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      addSelectionTypeFlags(root, state.selection_set);
    }

    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("find error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleSetRouteGuides(
    const WebSocketRequest& req,
    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    const std::string action = std::string(req.json.at("action").as_string());
    const std::string net_name
        = std::string(req.json.at("net_name").as_string());
    std::lock_guard<std::mutex> lock(state.route_guides_mutex);
    if (action == "clear") {
      state.route_guide_net_ids.clear();
    } else {
      odb::dbBlock* block = gen_->getBlock();
      odb::dbNet* net = block ? block->findNet(net_name.c_str()) : nullptr;
      if (net) {
        if (action == "add") {
          state.route_guide_net_ids.insert(net->getId());
        } else if (action == "remove") {
          state.route_guide_net_ids.erase(net->getId());
        }
      }
    }
    boost::json::object root;
    root["ok"] = 1;
    root["count"] = static_cast<int>(state.route_guide_net_ids.size());
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleSnap(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    TileVisibility vis;
    vis.parseFromJson(req.json);
    auto snap = gen_->snapAt(
        static_cast<int>(req.json.at("dbu_x").as_int64()),
        static_cast<int>(req.json.at("dbu_y").as_int64()),
        static_cast<int>(req.json.at("radius").as_int64()),
        static_cast<int>(req.json.at("point_threshold").as_int64()),
        req.json.at("horizontal").as_bool(),
        req.json.at("vertical").as_bool(),
        vis,
        arrayAsStringSet(req.json.at("visible_layers")));
    boost::json::object root;
    root["found"] = snap.found;
    if (snap.found) {
      const bool is_point = snap.edge.first == snap.edge.second;
      root["is_point"] = is_point;
      root["edge"] = boost::json::array{
          boost::json::array{snap.edge.first.x(), snap.edge.first.y()},
          boost::json::array{snap.edge.second.x(), snap.edge.second.y()}};
    }
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    std::string err = std::string("snap error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

static const char* ioTypeToDirection(odb::dbIoType io_type)
{
  if (io_type == odb::dbIoType::INPUT) {
    return "input";
  }
  if (io_type == odb::dbIoType::OUTPUT) {
    return "output";
  }
  return "inout";
}

// Result of classifying an instance against the schematic gate symbols the web
// viewer can draw.
struct GateClass
{
  // "and"/"nand"/"or"/"nor"/"xor"/"xnor"/"not"/"buf" for simple gates,
  // "aoi"/"oai" for compound and/or-invert gates, or "" when the cell is not a
  // recognised combinational gate.
  std::string kind;
  // For "aoi"/"oai" only: the input pin names of each first-level term (e.g.
  // AOI21 -> {{"A"}, {"B1", "B2"}}).  A one-pin term is a literal fed straight
  // into the second-level gate; multi-pin terms become an AND (aoi) or OR (oai)
  // sub-gate.  The viewer uses the pin names to align each input to its real
  // port.  Empty otherwise.
  std::vector<std::vector<std::string>> terms;
};

// Append the operands of `e`, flattening nested nodes that share `op`, so that
// e.g. or(or(a,b),c) yields {a, b, c}.
static void flattenFuncExpr(sta::FuncExpr* e,
                            sta::FuncExpr::Op op,
                            std::vector<sta::FuncExpr*>& out)
{
  if (e != nullptr && e->op() == op) {
    flattenFuncExpr(e->left(), op, out);
    flattenFuncExpr(e->right(), op, out);
  } else if (e != nullptr) {
    out.push_back(e);
  }
}

// Classify an and/or-invert tree: `func` is the expression with its leading
// inversion already removed, and `top` is its (and_ or or_) root operator.  An
// AOI is an OR of product terms (each a literal or an AND of literals); an OAI
// is the dual.  Returns the input pin names grouped by term, or an empty vector
// when the structure is anything else (e.g. a MUX, whose terms contain inverted
// literals).
static std::vector<std::vector<std::string>> classifyAoiOai(
    sta::FuncExpr* func,
    sta::FuncExpr::Op top)
{
  static constexpr int kMaxTerms = 4;
  static constexpr int kMaxInputs = 6;
  const sta::FuncExpr::Op term_op = (top == sta::FuncExpr::Op::and_)
                                        ? sta::FuncExpr::Op::or_
                                        : sta::FuncExpr::Op::and_;

  std::vector<sta::FuncExpr*> terms;
  flattenFuncExpr(func, top, terms);
  if (terms.size() < 2 || static_cast<int>(terms.size()) > kMaxTerms) {
    return {};
  }

  std::vector<std::vector<std::string>> groups;
  int total = 0;
  for (sta::FuncExpr* term : terms) {
    if (term->op() == sta::FuncExpr::Op::port) {
      groups.push_back({term->port()->name()});
      total += 1;
      continue;
    }
    if (term->op() != term_op) {
      return {};  // not a pure product/sum-of-literals term (e.g. a MUX)
    }
    std::vector<sta::FuncExpr*> literals;
    flattenFuncExpr(term, term_op, literals);
    std::vector<std::string> names;
    for (sta::FuncExpr* lit : literals) {
      if (lit->op() != sta::FuncExpr::Op::port) {
        return {};
      }
      names.push_back(lit->port()->name());
    }
    total += static_cast<int>(names.size());
    groups.push_back(std::move(names));
  }
  if (total > kMaxInputs) {
    return {};
  }
  return groups;
}

// Classify a leaf instance into a schematic gate symbol using its Liberty
// function, so the web schematic viewer can draw a recognisable gate outline
// (AND/OR/XOR/inverter/buffer, plus compound AOI/OAI) around the cell.
//
// The result is emitted only as a rendering *hint*: the cell still carries its
// real master name and pin names, so it keeps its instance label and port
// labels and falls back to a plain box if the viewer ignores the hint.
// Anything more complex (tristates, sequential cells, macros, XOR/MUX trees
// wider than two inputs, ...) returns an empty kind and is drawn as a box,
// which is the conventional way such cells appear on a schematic anyway.
static GateClass classifyGate(sta::dbNetwork* network, odb::dbInst* inst)
{
  GateClass result;
  if (network == nullptr) {
    return result;
  }
  sta::LibertyCell* cell = network->libertyCell(inst);
  if (cell == nullptr || cell->isSequential() || cell->isClockGate()
      || cell->isMacro()) {
    return result;
  }

  // Find the single functional output port.  Bail out on bussed cells or any
  // cell with more than one driven output (e.g. full adders, *_QN flops).
  sta::LibertyPort* out_port = nullptr;
  sta::FuncExpr* func = nullptr;
  sta::LibertyCellPortIterator port_iter(cell);
  while (port_iter.hasNext()) {
    sta::LibertyPort* port = port_iter.next();
    if (port->isBus() || port->hasMembers()) {
      return result;
    }
    if (!port->direction()->isOutput()) {
      continue;
    }
    if (port->tristateEnable() != nullptr) {
      return result;
    }
    sta::FuncExpr* f = port->function();
    if (f == nullptr) {
      continue;
    }
    if (out_port != nullptr) {
      return result;  // more than one functional output
    }
    out_port = port;
    func = f;
  }
  if (out_port == nullptr || func == nullptr) {
    return result;
  }

  // Peel leading inversions so AND/NAND, OR/NOR, XOR/XNOR and BUF/NOT share a
  // path.  Some cells (e.g. higher drive-strength variants) model the output
  // with stacked inverters, so the function can be nested NOTs like !(!(!(x)));
  // an odd count is inverting, an even count is not.
  bool inverting = false;
  while (func != nullptr && func->op() == sta::FuncExpr::Op::not_) {
    inverting = !inverting;
    func = func->left();
  }
  if (func == nullptr) {
    return result;
  }

  // Single input: buffer (Y = A) or inverter (Y = !A).
  if (func->op() == sta::FuncExpr::Op::port) {
    result.kind = inverting ? "not" : "buf";
    return result;
  }

  // A flat AND/OR/XOR of N plain ports is a basic N-input gate (N >= 2).  The
  // viewer derives the input count from the cell's ports, so only the kind is
  // emitted.
  const sta::FuncExpr::Op top = func->op();
  if (top == sta::FuncExpr::Op::and_ || top == sta::FuncExpr::Op::or_
      || top == sta::FuncExpr::Op::xor_) {
    std::vector<sta::FuncExpr*> operands;
    flattenFuncExpr(func, top, operands);
    bool all_ports = true;
    for (sta::FuncExpr* op : operands) {
      if (op->op() != sta::FuncExpr::Op::port) {
        all_ports = false;
        break;
      }
    }
    if (all_ports && operands.size() >= 2) {
      switch (top) {
        case sta::FuncExpr::Op::and_:
          result.kind = inverting ? "nand" : "and";
          return result;
        case sta::FuncExpr::Op::or_:
          result.kind = inverting ? "nor" : "or";
          return result;
        default:  // xor_
          result.kind = inverting ? "xnor" : "xor";
          return result;
      }
    }
  }

  // Compound and/or-invert gate (AOI/OAI).  These always invert, and their root
  // is an AND (OAI) or OR (AOI) of product/sum terms (at least one a sub-gate).
  if (inverting
      && (func->op() == sta::FuncExpr::Op::and_
          || func->op() == sta::FuncExpr::Op::or_)) {
    std::vector<std::vector<std::string>> terms
        = classifyAoiOai(func, func->op());
    if (!terms.empty()) {
      result.kind = (func->op() == sta::FuncExpr::Op::or_) ? "aoi" : "oai";
      result.terms = std::move(terms);
    }
  }
  return result;
}

// Emit one Yosys-format cell object for `inst` into `cells`.  The cell is
// always emitted verbatim (master name + real pin names) so netlistsvg renders
// it with its instance and port labels.  When it classifies as a standard
// logic gate, a non-standard "gate_kind" field is added that the viewer uses to
// draw a gate-shaped outline instead of a box.  Only nets present in
// `net_to_id` are wired; pins on other nets are skipped.
static void emitSchematicCell(boost::json::object& cells,
                              odb::dbInst* inst,
                              sta::dbNetwork* network,
                              odb::PtrMap<odb::dbNet, int>& net_to_id,
                              int& next_net_id)
{
  boost::json::object cell;
  cell["hide_name"] = 0;
  cell["type"] = inst->getMaster() ? inst->getMaster()->getName()
                                   : std::string("$unknown");
  cell["attributes"] = boost::json::object{};
  cell["parameters"] = boost::json::object{};

  const GateClass gate = classifyGate(network, inst);
  if (!gate.kind.empty()) {
    cell["gate_kind"] = gate.kind;
    if (!gate.terms.empty()) {
      boost::json::array terms;
      for (const std::vector<std::string>& group : gate.terms) {
        boost::json::array pins;
        for (const std::string& pin : group) {
          pins.push_back(boost::json::string(pin));
        }
        terms.push_back(std::move(pins));
      }
      cell["gate_terms"] = std::move(terms);
    }
  }

  // Emit every signal port, even dangling or out-of-scope ones.  A pin whose
  // net is absent from net_to_id (unconnected, or on a net outside the rendered
  // cone) gets a fresh synthetic bit id so netlistsvg still sees the pin and
  // draws the full symbol with a short dangling stub — otherwise a cell with
  // all such pins would collapse to a bare name label with no shape.  Synthetic
  // ids are deliberately left out of `netnames`, so they stay anonymous.
  // Power/ground pins are skipped: they aren't part of the logic schematic and
  // would otherwise render as spurious dangling stubs (and, for PDKs that mark
  // supplies as input/output, throw off the gate-symbol pin mapping).
  boost::json::object port_directions;
  boost::json::object connections;
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getSigType().isSupply()) {
      continue;
    }
    const std::string& pin = iterm->getMTerm()->getName();
    port_directions[pin] = ioTypeToDirection(iterm->getIoType());

    odb::dbNet* net = iterm->getNet();
    const int bit
        = (net && net_to_id.contains(net)) ? net_to_id[net] : next_net_id++;
    connections[pin] = boost::json::array{bit};
  }
  cell["port_directions"] = std::move(port_directions);
  cell["connections"] = std::move(connections);

  cells[inst->getName()] = std::move(cell);
}

WebSocketResponse SelectHandler::handleSchematicCone(
    const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  static constexpr int kMaxConeInsts = 150;
  static constexpr uint32_t kMaxNetFanout = 30;

  try {
    const std::string inst_name
        = std::string(req.json.at("inst_name").as_string());
    const int fanin_depth
        = static_cast<int>(req.json.at("fanin_depth").as_int64());
    const int fanout_depth
        = static_cast<int>(req.json.at("fanout_depth").as_int64());
    odb::dbBlock* block = gen_->getBlock();
    if (!block) {
      throw std::runtime_error("No block loaded");
    }

    odb::dbInst* target_inst = block->findInst(inst_name.c_str());
    if (!target_inst) {
      throw std::runtime_error("Instance not found: " + inst_name);
    }

    odb::PtrSet<odb::dbInst> all_insts;
    all_insts.insert(target_inst);
    bool cone_full = false;

    // Fanin BFS: follow input pins upstream to their driving instances.
    {
      std::vector<odb::dbInst*> level = {target_inst};
      odb::PtrSet<odb::dbNet> seen_nets;
      for (int d = 0; d < fanin_depth && !cone_full; ++d) {
        std::vector<odb::dbInst*> next_level;
        for (odb::dbInst* inst : level) {
          for (odb::dbITerm* iterm : inst->getITerms()) {
            if (iterm->getIoType() != odb::dbIoType::INPUT) {
              continue;
            }
            odb::dbNet* net = iterm->getNet();
            if (!net || seen_nets.contains(net)
                || net->getITerms().hasMoreThan(kMaxNetFanout)) {
              continue;
            }
            seen_nets.insert(net);
            for (odb::dbITerm* drv : net->getITerms()) {
              if (drv->getIoType() != odb::dbIoType::OUTPUT) {
                continue;
              }
              odb::dbInst* drv_inst = drv->getInst();
              if (all_insts.insert(drv_inst).second) {
                next_level.push_back(drv_inst);
                if (static_cast<int>(all_insts.size()) >= kMaxConeInsts) {
                  cone_full = true;
                  break;
                }
              }
            }
            if (cone_full) {
              break;
            }
          }
          if (cone_full) {
            break;
          }
        }
        level = next_level;
      }
    }

    // Fanout BFS: follow output pins downstream to their load instances.
    {
      std::vector<odb::dbInst*> level = {target_inst};
      odb::PtrSet<odb::dbNet> seen_nets;
      for (int d = 0; d < fanout_depth && !cone_full; ++d) {
        std::vector<odb::dbInst*> next_level;
        for (odb::dbInst* inst : level) {
          for (odb::dbITerm* iterm : inst->getITerms()) {
            if (iterm->getIoType() != odb::dbIoType::OUTPUT) {
              continue;
            }
            odb::dbNet* net = iterm->getNet();
            if (!net || seen_nets.contains(net)
                || net->getITerms().hasMoreThan(kMaxNetFanout)) {
              continue;
            }
            seen_nets.insert(net);
            for (odb::dbITerm* load : net->getITerms()) {
              if (load->getIoType() != odb::dbIoType::INPUT) {
                continue;
              }
              odb::dbInst* load_inst = load->getInst();
              if (all_insts.insert(load_inst).second) {
                next_level.push_back(load_inst);
                if (static_cast<int>(all_insts.size()) >= kMaxConeInsts) {
                  cone_full = true;
                  break;
                }
              }
            }
            if (cone_full) {
              break;
            }
          }
          if (cone_full) {
            break;
          }
        }
        level = next_level;
      }
    }

    // Collect all nets that touch any visited instance.
    odb::PtrMap<odb::dbNet, int> net_to_id;
    int next_net_id = 2;  // 0 = const-0, 1 = const-1 reserved by Yosys
    for (odb::dbInst* inst : all_insts) {
      for (odb::dbITerm* iterm : inst->getITerms()) {
        odb::dbNet* net = iterm->getNet();
        if (net && !net_to_id.contains(net)) {
          net_to_id[net] = next_net_id++;
        }
      }
    }

    boost::json::object top;
    top["attributes"] = boost::json::object{};

    boost::json::object ports;
    for (const auto& [net, _id] : net_to_id) {
      for (odb::dbBTerm* bterm : net->getBTerms()) {
        boost::json::object p;
        p["direction"] = ioTypeToDirection(bterm->getIoType());
        p["bits"] = boost::json::array{net_to_id[net]};
        ports[bterm->getName()] = std::move(p);
      }
    }
    top["ports"] = std::move(ports);

    sta::dbNetwork* network
        = gen_->getSta() ? gen_->getSta()->getDbNetwork() : nullptr;
    boost::json::object cells;
    for (odb::dbInst* inst : all_insts) {
      emitSchematicCell(cells, inst, network, net_to_id, next_net_id);
    }
    top["cells"] = std::move(cells);

    boost::json::object netnames;
    for (const auto& [net, net_id] : net_to_id) {
      boost::json::object n;
      n["hide_name"] = 0;
      n["bits"] = boost::json::array{net_id};
      n["attributes"] = boost::json::object{};
      netnames[net->getName()] = std::move(n);
    }
    top["netnames"] = std::move(netnames);

    boost::json::object root;
    root["modules"] = boost::json::object{{"top", std::move(top)}};
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleSchematicFull(
    const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;

  try {
    odb::dbBlock* block = gen_->getBlock();
    if (!block) {
      throw std::runtime_error("No block loaded");
    }

    odb::PtrMap<odb::dbNet, int> net_to_id;
    int next_net_id = 2;
    for (odb::dbNet* net : block->getNets()) {
      net_to_id[net] = next_net_id++;
    }

    boost::json::object top;
    top["attributes"] = boost::json::object{};

    boost::json::object ports;
    for (odb::dbBTerm* bterm : block->getBTerms()) {
      odb::dbNet* net = bterm->getNet();
      if (!net) {
        continue;
      }
      boost::json::object p;
      p["direction"] = ioTypeToDirection(bterm->getIoType());
      p["bits"] = boost::json::array{net_to_id[net]};
      ports[bterm->getName()] = std::move(p);
    }
    top["ports"] = std::move(ports);

    sta::dbNetwork* network
        = gen_->getSta() ? gen_->getSta()->getDbNetwork() : nullptr;
    boost::json::object cells;
    for (odb::dbInst* inst : block->getInsts()) {
      emitSchematicCell(cells, inst, network, net_to_id, next_net_id);
    }
    top["cells"] = std::move(cells);

    boost::json::object netnames;
    for (odb::dbNet* net : block->getNets()) {
      boost::json::object n;
      n["hide_name"] = 0;
      n["bits"] = boost::json::array{net_to_id[net]};
      n["attributes"] = boost::json::object{};
      netnames[net->getName()] = std::move(n);
    }
    top["netnames"] = std::move(netnames);

    boost::json::object root;
    root["modules"] = boost::json::object{{"top", std::move(top)}};
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleGet3DData(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;

  try {
    odb::dbChip* chip = gen_->getChip();
    if (!chip) {
      boost::json::object info;
      info["info"]
          = "No 3D chip data available. Load a multi-die design to "
            "use this view.";
      writePayload(resp, info);
      return resp;
    }

    boost::json::object root;
    boost::json::array chiplets;

    auto processInst = [&](auto& self,
                           odb::dbChipInst* inst,
                           int offset_x,
                           int offset_y,
                           int offset_z,
                           const std::string& parent_name) -> void {
      odb::dbChip* master_chip = inst->getMasterChip();
      if (master_chip && !master_chip->getChipInsts().empty()) {
        for (odb::dbChipInst* child : master_chip->getChipInsts()) {
          odb::Point3D loc = inst->getLoc();
          self(self,
               child,
               offset_x + loc.x(),
               offset_y + loc.y(),
               offset_z + loc.z(),
               parent_name + std::string(inst->getName()) + "/");
        }
      } else {
        boost::json::object obj;
        obj["name"] = parent_name + std::string(inst->getName());

        odb::Point3D loc = inst->getLoc();
        obj["x"] = offset_x + loc.x();
        obj["y"] = offset_y + loc.y();
        obj["z"] = offset_z + loc.z();

        int w = 0;
        int h = 0;
        int thickness = 0;
        if (master_chip) {
          w = master_chip->getWidth();
          h = master_chip->getHeight();
          thickness = master_chip->getThickness();
          // Fallback: use block bbox if chip has no explicit dimensions
          if (w == 0 || h == 0) {
            if (odb::dbBlock* block = master_chip->getBlock()) {
              if (odb::dbBox* block_bbox = block->getBBox()) {
                const odb::Rect bbox = block_bbox->getBox();
                if (w == 0) {
                  w = bbox.dx();
                }
                if (h == 0) {
                  h = bbox.dy();
                }
              }
            }
          }
        }
        constexpr int kDefaultChipWidth = 100000;
        constexpr int kDefaultChipHeight = 100000;
        constexpr int kDefaultChipThickness = 10000;
        obj["width"] = w > 0 ? w : kDefaultChipWidth;
        obj["height"] = h > 0 ? h : kDefaultChipHeight;
        obj["thickness"] = thickness > 0 ? thickness : kDefaultChipThickness;
        chiplets.emplace_back(std::move(obj));
      }
    };

    for (odb::dbChipInst* inst : chip->getChipInsts()) {
      processInst(processInst, inst, 0, 0, 0, "");
    }
    root["chiplets"] = std::move(chiplets);
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleSchematicInspect(
    const WebSocketRequest& req,
    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;

  try {
    consumeStaleSelection(state);
    const std::string inst_name
        = std::string(req.json.at("inst_name").as_string());
    odb::dbBlock* block = gen_->getBlock();
    if (!block) {
      throw std::runtime_error("No block loaded");
    }

    odb::dbInst* inst = block->findInst(inst_name.c_str());
    if (!inst) {
      throw std::runtime_error("Instance not found: " + inst_name);
    }

    gui::Selected sel = gui::DescriptorRegistry::instance()->makeSelected(inst);

    // STA's highlight() and getProperties() are not thread-safe;
    // serialize with other STA callers (timing, clock tree, tcl eval).
    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
    const bool use_dbu = jsonOr(req.json, "use_dbu", false);
    ScopedDbuFormat dbu_fmt(gen_->getDb(), use_dbu);

    int hl_group = -1;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      state.hover_rects.clear();
      state.timing_rects.clear();
      state.timing_lines.clear();
      setSelectionHighlights(state, sel);
      runDeselectAction(state.current_inspected, sel);
      state.current_inspected = sel;
      state.navigation_history.clear();
      hl_group = highlightGroupOfLocked(state, sel);
    }

    boost::json::object root;
    std::vector<gui::Selected> new_selectables;
    writeInspectPayload(root,
                        sel,
                        new_selectables,
                        /*can_navigate_back=*/false,
                        use_dbu,
                        gen_->getLogger(),
                        hl_group);
    {
      std::lock_guard<std::mutex> lock(state.selectables_mutex);
      state.selectables = std::move(new_selectables);
    }

    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

//------------------------------------------------------------------------------
// TclHandler
//------------------------------------------------------------------------------

TclHandler::TclHandler(std::shared_ptr<TclEvaluator> tcl_eval)
    : tcl_eval_(std::move(tcl_eval))
{
}

void TclHandler::registerRequests(RequestDispatcher& d)
{
  d.add("tcl_eval",
        WebSocketRequest::kTclEval,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleTclEval(req);
        });
  d.add("tcl_complete",
        WebSocketRequest::kTclComplete,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleTclComplete(req);
        });
}

WebSocketResponse TclHandler::handleTclEval(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    auto result = tcl_eval_->eval(std::string(req.json.at("cmd").as_string()));
    // tclExitHandler (web_serve.cpp) sets this sentinel as the Tcl
    // result whenever `exit`/`quit` is evaluated through the override —
    // whether typed bare in the browser or buried in `eval`/`source`.
    // Convert it to a clean shutdown signal for the browser; the actual
    // teardown is already requested by tclExitHandler via requestStop().
    const bool is_exit = (result.result == kExitResultMsg);
    boost::json::object root;
    if (is_exit) {
      tcl_eval_->logger->info(
          utl::WEB, 40, "Exit requested from web GUI; shutting down.");
      root["result"] = "Exiting OpenROAD.";
      root["is_error"] = false;
      root["action"] = "shutdown";
    } else {
      root["result"] = result.result;
      root["is_error"] = result.is_error;
    }
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TclHandler::handleTclComplete(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    const std::string line = std::string(req.json.at("line").as_string());
    const int cursor_pos
        = static_cast<int>(req.json.at("cursor_pos").as_int64());

    // The shared completer reads Tcl state via direct Tcl_Eval, so hold
    // the evaluator mutex for the same reasons regular eval requests do.
    ord::TclCompletion result;
    {
      std::lock_guard<std::mutex> lock(tcl_eval_->mutex);
      result = ord::completeTcl(tcl_eval_->interp, line, cursor_pos);
    }

    boost::json::object root;
    boost::json::array comp_arr;
    comp_arr.reserve(result.completions.size());
    for (const auto& c : result.completions) {
      comp_arr.emplace_back(c);
    }
    root["completions"] = std::move(comp_arr);
    root["mode"] = result.mode;
    root["prefix"] = result.prefix;
    root["replace_start"] = result.replace_start;
    root["replace_end"] = result.replace_end;
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

//------------------------------------------------------------------------------
// TimingHandler
//------------------------------------------------------------------------------

TimingHandler::TimingHandler(std::shared_ptr<TileGenerator> gen,
                             std::shared_ptr<TimingReport> timing_report,
                             std::shared_ptr<TclEvaluator> tcl_eval)
    : gen_(std::move(gen)),
      timing_report_(std::move(timing_report)),
      tcl_eval_(std::move(tcl_eval))
{
}

void TimingHandler::registerRequests(RequestDispatcher& d)
{
  d.add("timing_report",
        WebSocketRequest::kTimingReport,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleTimingReport(req);
        });
  d.add("timing_highlight",
        WebSocketRequest::kTimingHighlight,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleTimingHighlight(req, state);
        });
  d.add("timing_cone",
        WebSocketRequest::kTimingCone,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleTimingCone(req, state);
        });
  d.add("slack_histogram",
        WebSocketRequest::kSlackHistogram,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleSlackHistogram(req);
        });
  d.add("fanout_histogram",
        WebSocketRequest::kFanoutHistogram,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleFanoutHistogram(req);
        });
  d.add("net_length_histogram",
        WebSocketRequest::kNetLengthHistogram,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleNetLengthHistogram(req);
        });
  d.add("chart_filters",
        WebSocketRequest::kChartFilters,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleChartFilters(req);
        });
}

WebSocketResponse TimingHandler::handleTimingReport(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    std::lock_guard<std::mutex> lock(tcl_eval_->mutex);
    auto paths = timing_report_->getReport(
        req.json.at("is_setup").as_bool(),
        static_cast<int>(req.json.at("max_paths").as_int64()),
        static_cast<float>(jsonOr<double>(
            req.json, "slack_min", -std::numeric_limits<float>::max())),
        static_cast<float>(jsonOr<double>(
            req.json, "slack_max", std::numeric_limits<float>::max())));
    writePayload(resp, serializeTimingPaths(paths));
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TimingHandler::handleTimingHighlight(
    const WebSocketRequest& req,
    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    const int path_index
        = static_cast<int>(req.json.at("path_index").as_int64());
    std::vector<ColoredRect> new_rects;
    std::vector<FlightLine> new_lines;

    // path_index < 0 is the clear-highlight signal (no other fields used).
    if (path_index >= 0) {
      const bool is_setup = req.json.at("is_setup").as_bool();
      std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
      auto paths = timing_report_->getReport(is_setup);
      if (path_index < static_cast<int>(paths.size())) {
        odb::dbBlock* block = gen_->getBlock();
        collectTimingPathShapes(block, paths[path_index], new_rects, new_lines);

        const std::string pin_name
            = jsonOr<std::string>(req.json, "pin_name", "");
        if (!pin_name.empty()) {
          static const Color kStageColor{.r = 255, .g = 255, .b = 0, .a = 180};
          auto [iterm, bterm] = resolvePin(block, pin_name);

          odb::dbNet* net = nullptr;
          if (iterm) {
            net = iterm->getNet();
          } else if (bterm) {
            net = bterm->getNet();
          }

          if (net) {
            collectNetShapes(net,
                             iterm,
                             bterm,
                             nullptr,
                             nullptr,
                             kStageColor,
                             new_rects,
                             new_lines);
          }
        }
      }
    }

    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      state.timing_rects = std::move(new_rects);
      state.timing_lines = std::move(new_lines);
      clearSelectionHighlights(state);
    }

    const std::string json = "{\"ok\": true}";
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

namespace {

// Center of a cone pin in DBU: ITerm average pin location (fallback: instance
// center), or the first BPin box center for a BTerm.
odb::Point conePinCenter(const TimingConeNode& node)
{
  if (node.iterm) {
    int x = 0;
    int y = 0;
    if (node.iterm->getAvgXY(&x, &y)) {
      return {x, y};
    }
    const odb::Rect bbox = node.iterm->getInst()->getBBox()->getBox();
    return {(bbox.xMin() + bbox.xMax()) / 2, (bbox.yMin() + bbox.yMax()) / 2};
  }
  if (node.bterm) {
    for (odb::dbBPin* bpin : node.bterm->getBPins()) {
      const odb::Rect r = bpin->getBBox();
      return {(r.xMin() + r.xMax()) / 2, (r.yMin() + r.yMax()) / 2};
    }
  }
  return {0, 0};
}

}  // namespace

WebSocketResponse TimingHandler::handleTimingCone(const WebSocketRequest& req,
                                                  SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    const bool clear = jsonOr(req.json, "clear", false);
    const bool fanin = !clear && jsonOr(req.json, "fanin", false);
    const bool fanout = !clear && jsonOr(req.json, "fanout", false);

    std::vector<ColoredRect> new_rects;
    std::vector<FlightLine> new_lines;
    std::vector<TextLabel> new_labels;
    boost::json::object out;
    out["ok"] = true;

    if (!clear && (fanin || fanout)) {
      // Prefer an explicit pin; otherwise fall back to a representative pin of
      // the selected instance (output pin, else first signal pin) so the cone
      // can be triggered straight from a layout instance selection.
      std::string pin_name = jsonOr<std::string>(req.json, "pin_name", "");
      if (pin_name.empty()) {
        const std::string inst_name
            = jsonOr<std::string>(req.json, "inst_name", "");
        odb::dbBlock* block = gen_->getBlock();
        if (!inst_name.empty() && block != nullptr) {
          if (odb::dbInst* inst = block->findInst(inst_name.c_str())) {
            odb::dbITerm* chosen = nullptr;
            for (odb::dbITerm* iterm : inst->getITerms()) {
              if (iterm->getSigType().isSupply()) {
                continue;
              }
              if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
                chosen = iterm;
                break;
              }
              if (chosen == nullptr) {
                chosen = iterm;
              }
            }
            if (chosen != nullptr) {
              pin_name = chosen->getName();
            }
          }
        }
      }
      const int fanin_depth
          = static_cast<int>(jsonOr<int64_t>(req.json, "fanin_depth", 0));
      const int fanout_depth
          = static_cast<int>(jsonOr<int64_t>(req.json, "fanout_depth", 0));
      // color_mode "depth" colors by logic level; anything else colors by
      // slack (the Qt-parity default).
      const bool color_by_depth
          = jsonOr<std::string>(req.json, "color_mode", "slack") == "depth";

      TimingConeResult cone;
      {
        std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
        cone = timing_report_->computeTimingCone(
            pin_name, fanin, fanout, fanin_depth, fanout_depth);
      }
      if (!cone.ok) {
        resp.type = WebSocketResponse::kError;
        const std::string err = "timing cone: " + cone.error;
        resp.payload.assign(err.begin(), err.end());
        return resp;
      }

      // Color ratio in [0,1] fed to the Turbo spectrum.  Slack mode mirrors the
      // Qt renderer (worst slack → hot end); depth mode maps |level|.
      const double slack_range = cone.max_slack - cone.min_slack;
      int max_abs_depth = 1;
      for (const auto& node : cone.nodes) {
        max_abs_depth = std::max(max_abs_depth, std::abs(node.depth));
      }
      auto ratio_of = [&](const TimingConeNode& node) -> double {
        if (color_by_depth) {
          return static_cast<double>(std::abs(node.depth)) / max_abs_depth;
        }
        if (!cone.constrained || slack_range == 0.0 || !node.has_slack) {
          return 0.5;
        }
        return 1.0 - (node.slack - cone.min_slack) / slack_range;
      };

      // Highlight each instance once, colored by its worst-slack pin.
      // NOTE: odb deletes std::less<dbInst*>, so key on odb::PtrMap.  One hash
      // per node via try_emplace + the returned iterator (not contains + [] ).
      odb::PtrMap<odb::dbInst, const TimingConeNode*> worst_by_inst;
      for (const auto& node : cone.nodes) {
        if (node.inst == nullptr) {
          continue;
        }
        auto [it, inserted] = worst_by_inst.try_emplace(node.inst, &node);
        if (!inserted) {
          const TimingConeNode* cur = it->second;
          if (!cur->has_slack || (node.has_slack && node.slack < cur->slack)) {
            it->second = &node;
          }
        }
      }
      for (const auto& [inst, node] : worst_by_inst) {
        Color color = spectrumColor(ratio_of(*node), 150);
        new_rects.push_back({inst->getBBox()->getBox(), color, "", true});
      }

      // Pin centers, computed once per node (conePinCenter walks iterm
      // geometry, and a source is reused across all the sinks it drives).
      std::vector<odb::Point> centers(cone.nodes.size());
      for (size_t i = 0; i < cone.nodes.size(); ++i) {
        centers[i] = conePinCenter(cone.nodes[i]);
      }

      // Flight lines source(depth L) → sink(L+1), colored by source slack, and
      // per-pin depth labels.
      for (size_t i = 0; i < cone.nodes.size(); ++i) {
        const TimingConeNode& node = cone.nodes[i];
        const odb::Point& sink = centers[i];
        new_labels.push_back({sink,
                              std::to_string(node.depth),
                              Color{.r = 255, .g = 255, .b = 255, .a = 255}});
        for (const int src_idx : node.source_indices) {
          new_lines.push_back(
              {centers[src_idx],
               sink,
               spectrumColor(ratio_of(cone.nodes[src_idx]), 255)});
        }
      }

      out["node_count"] = static_cast<int64_t>(cone.nodes.size());
      out["constrained"] = cone.constrained;
      out["min_slack"] = cone.min_slack;
      out["max_slack"] = cone.max_slack;
      out["time_unit"] = cone.time_unit;
      out["color_mode"] = color_by_depth ? "depth" : "slack";
      out["max_depth"] = max_abs_depth;
    }

    {
      std::lock_guard<std::mutex> lock(state.cone_mutex);
      state.cone_rects = std::move(new_rects);
      state.cone_lines = std::move(new_lines);
      state.cone_labels = std::move(new_labels);
    }

    writePayload(resp, out);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TimingHandler::handleSlackHistogram(
    const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    std::lock_guard<std::mutex> lock(tcl_eval_->mutex);
    // `path_groups` (array) requests a stacked per-group breakdown; the scalar
    // `path_group` remains for the single/unfiltered case.
    std::vector<std::string> path_groups;
    if (auto* v = req.json.if_contains("path_groups")) {
      for (const auto& e : v->as_array()) {
        path_groups.emplace_back(e.as_string());
      }
    }
    auto histogram = timing_report_->getSlackHistogram(
        req.json.at("is_setup").as_bool(),
        jsonOr<std::string>(req.json, "path_group", ""),
        jsonOr<std::string>(req.json, "clock_name", ""),
        path_groups);
    writePayload(resp, serializeSlackHistogram(histogram));
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TimingHandler::handleFanoutHistogram(
    const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    std::lock_guard<std::mutex> lock(tcl_eval_->mutex);
    odb::dbBlock* block = gen_->getBlock();
    auto histogram = computeFanoutHistogram(block);
    writePayload(resp, serializeFanoutHistogram(histogram));
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TimingHandler::handleNetLengthHistogram(
    const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    std::lock_guard<std::mutex> lock(tcl_eval_->mutex);
    odb::dbBlock* block = gen_->getBlock();
    const bool use_dbu = jsonOr(req.json, "use_dbu", false);
    auto histogram = computeNetLengthHistogram(block, use_dbu);
    writePayload(resp, serializeNetLengthHistogram(histogram));
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TimingHandler::handleChartFilters(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    std::lock_guard<std::mutex> lock(tcl_eval_->mutex);
    auto filters = timing_report_->getChartFilters();
    writePayload(resp, serializeChartFilters(filters));
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

//------------------------------------------------------------------------------
// ClockTreeHandler
//------------------------------------------------------------------------------

ClockTreeHandler::ClockTreeHandler(
    std::shared_ptr<TileGenerator> gen,
    std::shared_ptr<ClockTreeReport> clock_report,
    std::shared_ptr<TclEvaluator> tcl_eval)
    : gen_(std::move(gen)),
      clock_report_(std::move(clock_report)),
      tcl_eval_(std::move(tcl_eval))
{
}

void ClockTreeHandler::registerRequests(RequestDispatcher& d)
{
  d.add("clock_tree",
        WebSocketRequest::kClockTree,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleClockTree(req);
        });
  d.add("clock_tree_highlight",
        WebSocketRequest::kClockTreeHighlight,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleClockTreeHighlight(req, state);
        });
}

WebSocketResponse ClockTreeHandler::handleClockTree(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    std::lock_guard<std::mutex> lock(tcl_eval_->mutex);
    auto clocks = clock_report_->getReport();
    boost::json::object root;
    boost::json::array clk_arr;
    clk_arr.reserve(clocks.size());
    for (const auto& clk : clocks) {
      boost::json::object o;
      o["name"] = clk.clock_name;
      o["min_arrival"] = clk.min_arrival;
      o["max_arrival"] = clk.max_arrival;
      o["time_unit"] = clk.time_unit;
      boost::json::array nodes;
      nodes.reserve(clk.nodes.size());
      for (const auto& n : clk.nodes) {
        boost::json::object node;
        node["id"] = n.id;
        node["parent_id"] = n.parent_id;
        node["name"] = n.name;
        node["pin_name"] = n.pin_name;
        node["type"] = ClockTreeNode::typeToString(n.type);
        node["arrival"] = n.arrival;
        node["delay"] = n.delay;
        node["fanout"] = n.fanout;
        node["level"] = n.level;
        node["dbu_x"] = n.dbu_x;
        node["dbu_y"] = n.dbu_y;
        nodes.emplace_back(std::move(node));
      }
      o["nodes"] = std::move(nodes);
      clk_arr.emplace_back(std::move(o));
    }
    root["clocks"] = std::move(clk_arr);
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse ClockTreeHandler::handleClockTreeHighlight(
    const WebSocketRequest& req,
    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    const std::string inst_name
        = std::string(req.json.at("inst_name").as_string());
    std::lock_guard<std::mutex> lock(state.selection_mutex);
    clearSelectionHighlights(state);
    state.timing_rects.clear();
    state.timing_lines.clear();

    if (!inst_name.empty()) {
      odb::dbBlock* block = gen_->getBlock();
      if (block) {
        odb::dbInst* inst = block->findInst(inst_name.c_str());
        if (inst) {
          state.highlight_rects.push_back(inst->getBBox()->getBox());
        }
      }
    }

    const std::string json = "{\"ok\": true}";
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

//------------------------------------------------------------------------------
// TileHandler
//------------------------------------------------------------------------------

TileHandler::TileHandler(std::shared_ptr<TileGenerator> gen)
    : gen_(std::move(gen))
{
}

void TileHandler::broadcastLabelsChanged()
{
  if (!broadcast_fn_) {
    return;
  }
  boost::json::object msg;
  msg["type"] = "labels_changed";
  // Carry the new set so a client can repaint from the push alone, the same
  // shape every label response already returns.
  msg["labels"] = gen_->labelsJson();
  broadcast_fn_(boost::json::serialize(msg));
}

void TileHandler::registerRequests(RequestDispatcher& d)
{
  d.add("tile",
        WebSocketRequest::kTile,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleTile(req, state);
        });
  d.add("bounds",
        WebSocketRequest::kBounds,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleTile(req, state);
        });
  d.add("tech",
        WebSocketRequest::kTech,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleTile(req, state);
        });
  // run_inline so the cancel runs on the read thread, ahead of the posted
  // tile render it is meant to abort.
  d.add(
      "cancel",
      WebSocketRequest::kCancel,
      [this](const WebSocketRequest& req, SessionState& state) {
        return handleCancel(req, state);
      },
      /*run_inline=*/true);
  d.add("module_hierarchy",
        WebSocketRequest::kModuleHierarchy,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleModuleHierarchy(req);
        });
  d.add("set_module_colors",
        WebSocketRequest::kSetModuleColors,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSetModuleColors(req, state);
        });
  d.add("heatmaps",
        WebSocketRequest::kHeatmaps,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleHeatMaps(req, state);
        });
  d.add("set_active_heatmap",
        WebSocketRequest::kSetActiveHeatmap,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSetActiveHeatMap(req, state);
        });
  d.add("set_heatmap",
        WebSocketRequest::kSetHeatmap,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSetHeatMap(req, state);
        });
  d.add("heatmap_tile",
        WebSocketRequest::kHeatmapTile,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleHeatMapTile(req, state);
        });
  d.add("overlay_tile",
        WebSocketRequest::kOverlayTile,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleOverlayTile(req, state);
        });
  d.add("add_label",
        WebSocketRequest::kAddLabel,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleAddLabel(req);
        });
  d.add("delete_label",
        WebSocketRequest::kDeleteLabel,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleDeleteLabel(req);
        });
  d.add("update_label",
        WebSocketRequest::kUpdateLabel,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleUpdateLabel(req);
        });
  d.add("clear_labels",
        WebSocketRequest::kClearLabels,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleClearLabels(req);
        });
  d.add("list_labels",
        WebSocketRequest::kListLabels,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleListLabels(req);
        });
}

void TileHandler::initializeHeatMaps(SessionState& state)
{
  std::lock_guard<std::mutex> lock(state.heatmap_mutex);
  state.heatmaps.clear();
  for (const auto& source_handle : gui::getRegisteredHeatMapSources()) {
    auto source = source_handle->createInstance();
    source->setChip(gen_->getChip());
    state.heatmaps[source_handle->getShortName()] = std::move(source);
  }
}

WebSocketResponse TileHandler::handleTile(const WebSocketRequest& req,
                                          SessionState& state)
{
  switch (req.type) {
    case WebSocketRequest::kBounds:
      return serializeBounds(req.id, *gen_);
    case WebSocketRequest::kTech:
      return serializeTech(req.id, *gen_);
    case WebSocketRequest::kTile:
      break;
    default: {
      WebSocketResponse resp;
      resp.id = req.id;
      resp.type = WebSocketResponse::kError;
      const std::string err = "Unknown request type";
      resp.payload.assign(err.begin(), err.end());
      return resp;
    }
  }

  const std::string layer = std::string(req.json.at("layer").as_string());
  // Validate before doing any work: a malformed request must not reach the
  // cache-key build below, let alone the renderer.
  int z = 0;
  int x = 0;
  int y = 0;
  if (std::string coord_error;
      !parseTileCoords(req.json, z, x, y, coord_error)) {
    return errorResponse(req.id, "bad tile request: " + coord_error);
  }

  // Skip a render the client abandoned while it sat queued (best-effort).
  {
    std::lock_guard<std::mutex> lock(state.cancelled_mutex);
    if (state.cancelled_ids.erase(req.id) > 0) {
      debugPrint(gen_->getLogger(),
                 utl::WEB,
                 "tile",
                 1,
                 "tile: id={} layer={} z/x/y={}/{}/{} cancelled before render",
                 req.id,
                 layer,
                 z,
                 x,
                 y);
      WebSocketResponse resp;
      resp.id = req.id;
      resp.type = WebSocketResponse::kError;
      const std::string msg = "cancelled";
      resp.payload.assign(msg.begin(), msg.end());
      return resp;
    }
  }

  TileVisibility vis;
  vis.parseFromJson(req.json);

  // Snapshot module colors for _modules layer
  std::map<uint32_t, Color> mod_colors;
  {
    std::lock_guard<std::mutex> lock(state.module_colors_mutex);
    mod_colors = state.module_colors;
  }
  const std::map<uint32_t, Color>* mod_ptr
      = mod_colors.empty() ? nullptr : &mod_colors;

  // Snapshot focus nets
  std::set<uint32_t> focus_nets;
  {
    std::lock_guard<std::mutex> lock(state.focus_nets_mutex);
    focus_nets = state.focus_net_ids;
  }
  const std::set<uint32_t>* focus_ptr
      = focus_nets.empty() ? nullptr : &focus_nets;

  // Base tiles no longer carry highlights — those are rendered by the
  // overlay tile layer.  Pass empty vectors so renderTileBuffer skips
  // drawHighlight / drawColoredHighlight / drawFlightLines / drawRouteGuides.
  static const std::vector<odb::Rect> no_rects;
  static const std::vector<odb::Polygon> no_polys;
  static const std::vector<ColoredRect> no_colored;
  static const std::vector<FlightLine> no_lines;

  const double dpr = quantizeDpr(jsonOr<double>(req.json, "dpr", 1.0));
  const int tile_px = quantizeTilePx(jsonOr<double>(req.json, "tile_px", 0.0));

  // A tile is cacheable only when it depends solely on the static design +
  // visibility + dpr — i.e. no per-session overlays are active.  That keeps
  // the cache session-independent and correct; overlay tiles render fresh.
  const bool cacheable = mod_ptr == nullptr && focus_ptr == nullptr
                         && !vis.debug && !vis.debug_renderers
                         && !vis.debug_live;

  std::string cache_key;
  if (cacheable) {
    // Key = the full render determinant: the request JSON minus the per-call
    // id, with dpr pinned to the quantized value actually rendered.
    // Selectability (the s_* flags and selectable_layers) does NOT affect the
    // rendered tile, so it is excluded — toggling "selectable" must not
    // invalidate the cache.
    boost::json::object key_obj = req.json;
    key_obj.erase("id");
    key_obj.erase("selectable_layers");
    std::vector<std::string> sel_keys;
    for (const auto& kv : key_obj) {
      const std::string_view k = kv.key();
      if (k.size() >= 2 && k[0] == 's' && k[1] == '_') {
        sel_keys.emplace_back(k);
      }
    }
    for (const std::string& k : sel_keys) {
      key_obj.erase(k);
    }
    key_obj["dpr"] = dpr;
    // Pinned like dpr: two clients on different displays ask for different
    // pixel counts of the same tile, and they are different images.
    key_obj["tile_px"] = tile_px;
    cache_key = boost::json::serialize(key_obj);
    std::vector<unsigned char> cached;
    if (gen_->tileCacheGet(cache_key, cached)) {
      WebSocketResponse resp;
      resp.id = req.id;
      resp.type = WebSocketResponse::kPng;
      resp.payload = std::move(cached);
      debugPrint(gen_->getLogger(),
                 utl::WEB,
                 "tile",
                 1,
                 "tile: id={} layer={} z/x/y={}/{}/{} dpr={} tile_px={} "
                 "cache=hit bytes={}",
                 req.id,
                 layer,
                 z,
                 x,
                 y,
                 dpr,
                 tile_px,
                 resp.payload.size());
      return resp;
    }
  }

  WebSocketResponse resp = renderTile(req.id,
                                      layer,
                                      z,
                                      x,
                                      y,
                                      vis,
                                      *gen_,
                                      no_rects,
                                      no_polys,
                                      no_colored,
                                      no_lines,
                                      mod_ptr,
                                      focus_ptr,
                                      nullptr,
                                      dpr,
                                      tile_px);
  if (cacheable && resp.type == WebSocketResponse::kPng) {
    gen_->tileCachePut(std::move(cache_key), resp.payload);
  }

  debugPrint(gen_->getLogger(),
             utl::WEB,
             "tile",
             1,
             "tile: id={} layer={} z/x/y={}/{}/{} dpr={} tile_px={} cache={} "
             "module_colors={} focus_nets={} detailed={} bytes={}",
             req.id,
             layer,
             z,
             x,
             y,
             dpr,
             tile_px,
             cacheable ? "miss" : "off",
             mod_ptr != nullptr ? mod_colors.size() : 0,
             focus_ptr != nullptr ? focus_nets.size() : 0,
             vis.detailed,
             resp.payload.size());

  return resp;
}

WebSocketResponse TileHandler::handleOverlayTile(const WebSocketRequest& req,
                                                 SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  // Overlay requests run on a bare io_context thread with no dispatcher-level
  // try/catch, so convert malformed-request exceptions (e.g. a non-bool
  // toggle flag) into an error response instead of terminating the server.
  try {
    // Validate first: below this point the handler mutates session state (the
    // "Flywires only" latch) and takes several mutexes, and a request that is
    // going to be refused must not do any of that.
    int z = 0;
    int x = 0;
    int y = 0;
    if (std::string coord_error;
        !parseTileCoords(req.json, z, x, y, coord_error)) {
      return errorResponse(req.id, "bad overlay request: " + coord_error);
    }

    // Drop stale highlight/hover shapes (and the dangling Selected objects
    // behind them) if a destroy invalidated the selection.
    consumeStaleSelection(state);

    // Re-derive the selection highlights when any of:
    //  - the "Flywires only" toggle flipped (so the change takes effect
    //    without re-selecting) — but only while the highlights are live: a
    //    flip after an explicit "clear highlights" must not resurrect them;
    //  - an edit moved a selected object.  The mover may be another session,
    //    whose broadcast only tells this client to redraw — the shapes it
    //    would redraw from live here, so they have to be rebuilt on this
    //    side or the highlight stays at the old placement;
    //  - debug renderers are active — instance positions change between
    //    frames, so the highlight must track the moving instance.
    const bool flywires_only = jsonOr(req.json, "flywires_only", false);
    const bool debug_renderers = jsonOr(req.json, "debug_renderers", false);
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      const bool flipped = (flywires_only != state.flywires_only);
      state.flywires_only = flywires_only;
      const bool live
          = state.highlight_source != SessionState::HighlightSource::kNone;
      // exchange() once: both the selection and the group shapes below are
      // rebuilt from this one notification.
      const bool geometry_stale
          = state.highlight_geometry_stale.exchange(false);
      if (((flipped || geometry_stale) && live)
          || (debug_renderers && state.current_inspected)) {
        // Re-derive from the SAME source the shapes came from.  Preferring
        // current_inspected unconditionally would drop every other item of a
        // shift+click multi-selection on the first flip, with no way back. With
        // no prior highlight the source is kNone, which only the debug-renderer
        // disjunct can reach — there the inspected object is what to track.
        if (state.highlight_source
            == SessionState::HighlightSource::kSelectionSet) {
          setSelectionSetHighlights(state);
        } else {
          setSelectionHighlights(state, state.current_inspected);
        }
      }
      // Group shapes are derived the same way and go stale for the same two
      // reasons.  Unconditional on `live`: groups are a state of their own
      // and are not cleared with the selection.
      if (flipped || geometry_stale) {
        rebuildHighlightGroupShapesLocked(state);
      }
    }

    // Snapshot current highlight state.
    // "Highlight selected" (Misc toggle, default on — Qt parity) gates the
    // current selection's highlight as a whole: its rects, its polys and its
    // flywires.  Leaving the flywires in would draw half a highlight for a
    // toggle whose label promises none.  Hover is a separate state and the
    // timing shapes have their own controls, so both are always drawn; the
    // selection itself stays active server-side regardless.
    const bool highlight_selected
        = jsonOr(req.json, "highlight_selected", true);
    std::vector<odb::Rect> rects;
    std::vector<odb::Polygon> polys;
    std::vector<ColoredRect> colored;
    std::vector<ColoredPolygon> colored_polys;
    std::vector<FlightLine> lines;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      if (highlight_selected) {
        rects = state.highlight_rects;
        polys = state.highlight_polys;
        lines = state.highlight_lines;
      }
      rects.insert(
          rects.end(), state.hover_rects.begin(), state.hover_rects.end());
      colored = state.timing_rects;
      lines.insert(
          lines.end(), state.timing_lines.begin(), state.timing_lines.end());
    }

    // Merge DRC overlay shapes
    {
      std::lock_guard<std::mutex> lock(state.drc_mutex);
      colored.insert(
          colored.end(), state.drc_rects.begin(), state.drc_rects.end());
      lines.insert(lines.end(), state.drc_lines.begin(), state.drc_lines.end());
    }

    // Merge timing-cone overlay shapes (instances, flight lines, depth
    // labels).
    std::vector<TextLabel> labels;
    {
      std::lock_guard<std::mutex> lock(state.cone_mutex);
      colored.insert(
          colored.end(), state.cone_rects.begin(), state.cone_rects.end());
      lines.insert(
          lines.end(), state.cone_lines.begin(), state.cone_lines.end());
      labels = state.cone_labels;
    }

    // Merge user text labels (2.12) unless the client hid the "Labels" group.
    // Absent flag → draw (default visible).
    if (jsonOr(req.json, "draw_labels", true)) {
      std::vector<TextLabel> user_labels = gen_->labelsForDraw();
      labels.insert(labels.end(), user_labels.begin(), user_labels.end());
    }
    {
      // Highlight groups append last so they paint on top (the Qt GUI
      // draws drawHighlighted "always last so on top").  They are a state of
      // their own, not part of the current selection, so "Highlight selected"
      // does not gate them — same as in the Qt GUI.
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      colored.insert(colored.end(),
                     state.highlight_group_rects.begin(),
                     state.highlight_group_rects.end());
      colored_polys = state.highlight_group_polys;
      lines.insert(lines.end(),
                   state.highlight_group_lines.begin(),
                   state.highlight_group_lines.end());
    }

    // Snapshot the nets whose route guides should be drawn.  Always include
    // the per-net selections (inspector "Show route guides", a web-only
    // finer control).  When the global "Focused nets guides" toggle is on,
    // also include every focused net — mirroring the Qt GUI toggle, which
    // draws the guides of all focused nets at once.
    const bool focused_nets_guides
        = jsonOr(req.json, "focused_nets_guides", false);
    std::set<uint32_t> route_guides;
    {
      std::lock_guard<std::mutex> lock(state.route_guides_mutex);
      route_guides = state.route_guide_net_ids;
    }
    if (focused_nets_guides) {
      std::lock_guard<std::mutex> lock(state.focus_nets_mutex);
      route_guides.insert(state.focus_net_ids.begin(),
                          state.focus_net_ids.end());
    }
    const std::set<uint32_t>* route_guide_ptr
        = route_guides.empty() ? nullptr : &route_guides;

    // Parse visible layers so route guides respect layer visibility.
    // has_vis_layers=true means the field was present (even if empty,
    // which means "all layers hidden" — matching pin-marker semantics).
    bool has_vis_layers = false;
    std::set<std::string> vis_layers;
    if (auto it = req.json.find("visible_layers"); it != req.json.end()) {
      has_vis_layers = true;
      const auto& arr = it->value().as_array();
      for (const auto& elem : arr) {
        vis_layers.emplace(elem.as_string());
      }
    }

    // Same sizing contract as layer tiles: an overlay rendered at a different
    // size than the tiles beneath it is blurry and misregistered against them.
    const double dpr = quantizeDpr(jsonOr<double>(req.json, "dpr", 1.0));
    const int tile_px
        = quantizeTilePx(jsonOr<double>(req.json, "tile_px", 0.0));

    resp.type = WebSocketResponse::kPng;
    resp.payload = gen_->generateOverlayTile(z,
                                             x,
                                             y,
                                             rects,
                                             polys,
                                             colored,
                                             lines,
                                             route_guide_ptr,
                                             has_vis_layers,
                                             vis_layers,
                                             dpr,
                                             tile_px,
                                             colored_polys,
                                             labels);

    // The selection highlight the client draws over the layer tiles comes from
    // here, so the shape counts say whether a "missing" highlight was never
    // collected server-side or just not drawn.
    debugPrint(gen_->getLogger(),
               utl::WEB,
               "tile",
               1,
               "overlay tile: id={} z/x/y={}/{}/{} dpr={} tile_px={} rects={} "
               "polys={} colored={} lines={} labels={} route_guide_nets={} "
               "flywires_only={} bytes={}",
               req.id,
               z,
               x,
               y,
               dpr,
               tile_px,
               rects.size(),
               polys.size(),
               colored.size(),
               lines.size(),
               labels.size(),
               route_guides.size(),
               flywires_only,
               resp.payload.size());
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

namespace {

// Parse an optional {r,g,b,a} color object; defaults to opaque white.
Color parseLabelColor(const boost::json::object& obj)
{
  Color c{.r = 255, .g = 255, .b = 255, .a = 255};
  if (auto* v = obj.if_contains("color")) {
    const auto& co = v->as_object();
    c.r = static_cast<unsigned char>(jsonOr<int64_t>(co, "r", 255));
    c.g = static_cast<unsigned char>(jsonOr<int64_t>(co, "g", 255));
    c.b = static_cast<unsigned char>(jsonOr<int64_t>(co, "b", 255));
    c.a = static_cast<unsigned char>(jsonOr<int64_t>(co, "a", 255));
  }
  return c;
}

// Common label attributes shared by add_label and update_label (name is
// handled separately since only add auto-generates it).
struct LabelFields
{
  odb::Point pos;
  std::string text;
  int size;
  std::string anchor;
  Color color;
};

LabelFields parseLabelFields(const boost::json::object& obj)
{
  std::string anchor = jsonOr<std::string>(obj, "anchor", "center");
  if (anchor.empty()) {
    anchor = "center";
  }
  // The Inspector offers a fixed list, so an unknown name here means a
  // hand-written API call.  Refuse it: centring a label the caller asked to
  // corner-anchor is a silent wrong answer.  Throwing becomes an error
  // response via the handler's catch.
  if (!isValidAnchor(anchor)) {
    throw std::runtime_error("anchor not recognized: " + anchor);
  }
  return {.pos = odb::Point(static_cast<int>(obj.at("x").as_int64()),
                            static_cast<int>(obj.at("y").as_int64())),
          .text = std::string(obj.at("text").as_string()),
          .size = static_cast<int>(jsonOr<int64_t>(obj, "size", 0)),
          .anchor = anchor,
          .color = parseLabelColor(obj)};
}

}  // namespace

WebSocketResponse TileHandler::handleAddLabel(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    const LabelFields f = parseLabelFields(req.json);
    const std::string name = jsonOr<std::string>(req.json, "name", "");

    const std::string result
        = gen_->addLabel(f.pos, f.text, f.color, f.size, f.anchor, name);
    boost::json::object root;
    root["ok"] = !result.empty();
    root["name"] = result;
    root["labels"] = gen_->labelsJson();
    writePayload(resp, root);
    if (!result.empty()) {
      broadcastLabelsChanged();
    }
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TileHandler::handleDeleteLabel(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    const std::string name(req.json.at("name").as_string());
    const bool ok = gen_->deleteLabel(name);
    boost::json::object root;
    root["ok"] = ok;
    root["labels"] = gen_->labelsJson();
    writePayload(resp, root);
    if (ok) {
      broadcastLabelsChanged();
    }
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TileHandler::handleUpdateLabel(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    const std::string name(req.json.at("name").as_string());
    const LabelFields f = parseLabelFields(req.json);

    const bool ok
        = gen_->updateLabel(name, f.pos, f.text, f.color, f.size, f.anchor);
    boost::json::object root;
    root["ok"] = ok;
    root["labels"] = gen_->labelsJson();
    writePayload(resp, root);
    if (ok) {
      broadcastLabelsChanged();
    }
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TileHandler::handleClearLabels(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  gen_->clearLabels();
  boost::json::object root;
  root["ok"] = true;
  root["labels"] = gen_->labelsJson();
  writePayload(resp, root);
  broadcastLabelsChanged();
  return resp;
}

WebSocketResponse TileHandler::handleListLabels(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  boost::json::object root;
  root["labels"] = gen_->labelsJson();
  // The Inspector's Anchor picker is built from this, so the choices it
  // offers are exactly the ones parseLabelFields will accept.  Sent with the
  // startup list rather than hard-coded client-side, where it could drift.
  boost::json::array anchors;
  for (const std::string& name : anchorNames()) {
    anchors.emplace_back(name);
  }
  root["anchors"] = std::move(anchors);
  writePayload(resp, root);
  return resp;
}

WebSocketResponse TileHandler::handleCancel(const WebSocketRequest& req,
                                            SessionState& state)
{
  // Cap so a long browsing session whose cancels never match a queued render
  // can't grow the set without bound; trimming the oldest (lowest) ids only
  // costs an occasional missed cancel (the render proceeds, which is correct).
  constexpr size_t kCancelledCap = 4096;
  size_t requested = 0;
  size_t pending = 0;
  {
    std::lock_guard<std::mutex> lock(state.cancelled_mutex);
    if (const auto* ids = req.json.if_contains("cancel_ids")) {
      for (const auto& v : ids->as_array()) {
        state.cancelled_ids.insert(static_cast<uint32_t>(v.as_int64()));
        ++requested;
      }
    } else {
      state.cancelled_ids.insert(
          static_cast<uint32_t>(jsonOr<int64_t>(req.json, "cancel_id", 0)));
      requested = 1;
    }
    while (state.cancelled_ids.size() > kCancelledCap) {
      state.cancelled_ids.erase(state.cancelled_ids.begin());
    }
    pending = state.cancelled_ids.size();
  }
  debugPrint(gen_->getLogger(),
             utl::WEB,
             "tile",
             1,
             "cancel: id={} cancelled={} pending={}",
             req.id,
             requested,
             pending);
  // Minimal ack.  The client does not track the cancel message in `pending`,
  // so this response is harmlessly ignored.
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  const std::string ok = "{\"cancelled\":1}";
  resp.payload.assign(ok.begin(), ok.end());
  return resp;
}

WebSocketResponse TileHandler::handleModuleHierarchy(
    const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    odb::dbBlock* block = gen_->getBlock();
    HierarchyReport report(block, gen_->getSta());
    auto result = report.getReport();
    writePayload(resp, serializeHierarchyResult(result));
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TileHandler::handleSetModuleColors(
    const WebSocketRequest& req,
    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  std::map<uint32_t, Color> colors;
  const std::string data = std::string(req.json.at("colors").as_string());
  if (!data.empty()) {
    size_t pos = 0;
    while (pos < data.size()) {
      size_t colon = data.find(':', pos);
      if (colon == std::string::npos) {
        break;
      }
      const uint32_t mod_id
          = static_cast<uint32_t>(std::stoul(data.substr(pos, colon - pos)));
      pos = colon + 1;

      auto next_num = [&]() -> int {
        size_t end = data.find_first_of(",;", pos);
        if (end == std::string::npos) {
          end = data.size();
        }
        const int val = std::stoi(data.substr(pos, end - pos));
        pos = end + 1;
        return val;
      };

      const uint8_t r = static_cast<uint8_t>(next_num());
      const uint8_t g = static_cast<uint8_t>(next_num());
      const uint8_t b = static_cast<uint8_t>(next_num());
      const uint8_t a = static_cast<uint8_t>(next_num());
      colors[mod_id] = Color{.r = r, .g = g, .b = b, .a = a};
    }
  }

  const int count = static_cast<int>(colors.size());
  {
    std::lock_guard<std::mutex> lock(state.module_colors_mutex);
    state.module_colors = std::move(colors);
  }

  const std::string ok = R"({"ok":1,"count":)" + std::to_string(count) + "}";
  resp.payload.assign(ok.begin(), ok.end());
  return resp;
}

WebSocketResponse TileHandler::handleHeatMaps(const WebSocketRequest& req,
                                              SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    const std::string json = buildHeatMapsPayloadLocked(state);
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TileHandler::handleSetActiveHeatMap(
    const WebSocketRequest& req,
    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    const std::string name = std::string(req.json.at("name").as_string());
    std::lock_guard<std::mutex> lock(state.heatmap_mutex);
    if (!state.active_heatmap.empty()) {
      auto current = state.heatmaps.find(state.active_heatmap);
      if (current != state.heatmaps.end()) {
        current->second->onHide();
      }
    }

    state.active_heatmap.clear();
    if (!name.empty()) {
      auto next = state.heatmaps.find(name);
      if (next == state.heatmaps.end()) {
        throw std::runtime_error("invalid heat map");
      }
      state.active_heatmap = name;
      next->second->onShow();
    }

    const std::string json = buildHeatMapsPayloadLocked(state);
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TileHandler::handleSetHeatMap(const WebSocketRequest& req,
                                                SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    const std::string name = std::string(req.json.at("name").as_string());
    const std::string option = std::string(req.json.at("option").as_string());
    std::lock_guard<std::mutex> lock(state.heatmap_mutex);
    auto source_itr = state.heatmaps.find(name);
    if (source_itr == state.heatmaps.end()) {
      throw std::runtime_error("invalid heat map");
    }

    auto& source = *source_itr->second;
    if (option == "rebuild") {
      source.destroyMap();
      source.ensureMap();
    } else {
      auto settings = source.getSettings();
      auto setting_itr = settings.find(option);
      if (setting_itr == settings.end()) {
        throw std::runtime_error("invalid heat map option");
      }

      const auto& current_value = setting_itr->second;
      const auto& value_v = req.json.at("value");
      if (std::holds_alternative<bool>(current_value)) {
        settings[option] = value_v.as_bool();
      } else if (std::holds_alternative<int>(current_value)) {
        // The frontend's addNumber control runs every value through
        // parseFloat, so int settings can arrive as JSON doubles.  Accept
        // either and round.
        settings[option]
            = value_v.is_int64()
                  ? static_cast<int>(value_v.get_int64())
                  : static_cast<int>(std::round(value_v.as_double()));
      } else if (std::holds_alternative<double>(current_value)) {
        settings[option] = value_v.as_double();
      } else {
        settings[option] = std::string(value_v.as_string());
      }
      source.setSettings(settings);
    }

    const std::string json = buildHeatMapsPayloadLocked(state);
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TileHandler::handleHeatMapTile(const WebSocketRequest& req,
                                                 SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kPng;
  try {
    const std::string req_name = std::string(req.json.at("name").as_string());
    int z = 0;
    int x = 0;
    int y = 0;
    if (std::string coord_error;
        !parseTileCoords(req.json, z, x, y, coord_error)) {
      return errorResponse(req.id, "bad heat map request: " + coord_error);
    }
    // Same sizing contract as layer tiles: the client states the device-pixel
    // square, so the heat map is as crisp as the layers beneath it.
    const double dpr = quantizeDpr(jsonOr<double>(req.json, "dpr", 1.0));
    const int tile_px
        = quantizeTilePx(jsonOr<double>(req.json, "tile_px", 0.0));
    std::shared_ptr<gui::HeatMapDataSource> source;
    {
      std::lock_guard<std::mutex> lock(state.heatmap_mutex);
      const std::string name
          = req_name.empty() ? state.active_heatmap : req_name;
      auto source_itr = state.heatmaps.find(name);
      if (source_itr == state.heatmaps.end()) {
        throw std::runtime_error("invalid heat map");
      }
      source = source_itr->second;
    }
    resp.payload = gen_->generateHeatMapTile(*source, z, x, y, dpr, tile_px);
    debugPrint(gen_->getLogger(),
               utl::WEB,
               "tile",
               1,
               "heatmap tile: id={} name={} z/x/y={}/{}/{} dpr={} tile_px={} "
               "bytes={}",
               req.id,
               source->getName(),
               z,
               x,
               y,
               dpr,
               tile_px,
               resp.payload.size());
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
    debugPrint(gen_->getLogger(),
               utl::WEB,
               "tile",
               1,
               "heatmap tile: id={} failed: {}",
               req.id,
               e.what());
  }
  return resp;
}

WebSocketResponse handleListDir(const WebSocketRequest& req)
{
  namespace fs = std::filesystem;

  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;

  try {
    const std::string dir_str = std::string(req.json.at("path").as_string());
    fs::path dir_path
        = dir_str.empty() ? fs::current_path() : fs::path(dir_str);
    dir_path = fs::canonical(dir_path);

    struct Entry
    {
      std::string name;
      bool is_dir;
      std::uintmax_t size;
    };
    std::vector<Entry> entries;

    for (const auto& entry : fs::directory_iterator(
             dir_path, fs::directory_options::skip_permission_denied)) {
      const auto& name = entry.path().filename().string();
      // Skip hidden files/directories.
      if (!name.empty() && name[0] == '.') {
        continue;
      }
      bool is_dir = entry.is_directory();
      std::uintmax_t size = 0;
      if (!is_dir) {
        std::error_code ec;
        size = entry.file_size(ec);
        if (ec) {
          size = 0;
        }
      }
      entries.push_back({name, is_dir, size});
    }

    // Sort: directories first, then alphabetical within each group.
    std::ranges::sort(entries, [](const Entry& a, const Entry& b) {
      if (a.is_dir != b.is_dir) {
        return a.is_dir > b.is_dir;
      }
      return a.name < b.name;
    });

    boost::json::object root;
    root["path"] = dir_path.string();
    root["parent"] = dir_path.parent_path().string();
    boost::json::array arr;
    arr.reserve(entries.size());
    for (const auto& entry : entries) {
      boost::json::object o;
      o["name"] = entry.name;
      o["is_dir"] = entry.is_dir;
      if (!entry.is_dir) {
        o["size"] = static_cast<int>(entry.size);
      }
      arr.emplace_back(std::move(o));
    }
    root["entries"] = std::move(arr);
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    std::string err = std::string("list_dir error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

//------------------------------------------------------------------------------
// DRCHandler
//------------------------------------------------------------------------------

DRCHandler::DRCHandler(std::shared_ptr<TileGenerator> gen)
    : gen_(std::move(gen))
{
}

void DRCHandler::registerRequests(RequestDispatcher& d)
{
  d.add("drc_categories",
        WebSocketRequest::kDrcCategories,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleDRCCategories(req);
        });
  d.add("drc_markers",
        WebSocketRequest::kDrcMarkers,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleDRCMarkers(req, state);
        });
  d.add("drc_load_report",
        WebSocketRequest::kDrcLoadReport,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleDRCLoadReport(req, state);
        });
  d.add("drc_update_marker",
        WebSocketRequest::kDrcUpdateMarker,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleDRCUpdateMarker(req, state);
        });
  d.add("drc_update_category_visibility",
        WebSocketRequest::kDrcUpdateCategoryVisibility,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleDRCUpdateCategoryVisibility(req, state);
        });
  d.add("drc_highlight",
        WebSocketRequest::kDrcHighlight,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleDRCHighlight(req, state);
        });
}

std::pair<odb::dbBlock*, odb::dbChip*> DRCHandler::getBlockAndChip()
{
  odb::dbChip* chip = gen_->getChip();
  if (!chip) {
    throw std::runtime_error("No chip loaded");
  }
  odb::dbBlock* block = chip->getBlock();
  return {block, chip};
}

odb::dbMarker* DRCHandler::findMarkerById(SessionState& state,
                                          odb::dbChip* chip,
                                          int marker_id)
{
  std::lock_guard<std::mutex> lock(state.drc_mutex);
  if (state.active_drc_category.empty()) {
    return nullptr;
  }
  odb::dbMarkerCategory* category
      = chip->findMarkerCategory(state.active_drc_category.c_str());
  if (!category) {
    return nullptr;
  }
  for (odb::dbMarker* marker : category->getAllMarkers()) {
    if (static_cast<int>(marker->getId()) == marker_id) {
      return marker;
    }
  }
  return nullptr;
}

void DRCHandler::refreshDRCOverlay(SessionState& state)
{
  // Must be called with drc_mutex already held.
  state.drc_rects.clear();
  state.drc_lines.clear();

  odb::dbChip* chip = gen_->getChip();
  if (!chip || state.active_drc_category.empty()) {
    return;
  }
  odb::dbBlock* block = chip->getBlock();

  odb::dbMarkerCategory* category
      = chip->findMarkerCategory(state.active_drc_category.c_str());
  if (!category) {
    return;
  }

  // Match the Qt GUI rendering style (dbDescriptors.cpp paintMarker +
  // drcWidget.cpp DRCRenderer::drawObjects):
  //   pen = white (solid), brush = white alpha 50 diagonal cross-hatch.
  // We approximate: Rect/Polygon/Cuboid → filled semi-transparent rect
  // with solid outline.  Line → drawn as a line.  Point → drawn as X.
  // When the marker bbox is too small (< min_box DBU), draw an X at
  // the center instead, matching the GUI's min_box fallback.
  // Color matches Qt GUI's Painter::kHighlight (yellow).
  const Color yellow_fill{.r = 255, .g = 255, .b = 0, .a = 100};
  const Color yellow_line{.r = 255, .g = 255, .b = 0, .a = 255};

  // min_box: cached tech pitch as "minimum visible size" threshold.
  // Default to 200 DBU (0.2um at 1000 dbu/um) if no routing layer available.
  if (min_box_ < 0) {
    constexpr int kDefaultMinBox = 200;
    min_box_ = kDefaultMinBox;
    if (block) {
      odb::dbTech* tech = block->getTech();
      if (tech) {
        for (odb::dbTechLayer* layer : tech->getLayers()) {
          if (layer && layer->getType() == odb::dbTechLayerType::ROUTING) {
            const int pitch = layer->getPitch();
            if (pitch > 0) {
              min_box_ = pitch;
              break;
            }
          }
        }
      }
    }
  }
  const int min_box = min_box_;

  auto emitX = [&](int cx, int cy, int half) {
    // Two diagonal lines forming an X, matching GUI's painter.drawX().
    state.drc_lines.push_back({odb::Point(cx - half, cy - half),
                               odb::Point(cx + half, cy + half),
                               yellow_line});
    state.drc_lines.push_back({odb::Point(cx - half, cy + half),
                               odb::Point(cx + half, cy - half),
                               yellow_line});
  };

  for (odb::dbMarker* marker : category->getAllMarkers()) {
    if (!marker->isVisible()) {
      continue;
    }

    const odb::Rect bbox = marker->getBBox();

    // GUI fallback: if bbox is too small, draw X at center instead of
    // individual shapes (dbDescriptors.cpp paintMarker, min_box check).
    if (bbox.maxDXDY() < min_box) {
      const int cx = bbox.xMin() + bbox.dx() / 2;
      const int cy = bbox.yMin() + bbox.dy() / 2;
      emitX(cx, cy, min_box / 2);
      continue;
    }

    const auto& shapes = marker->getShapes();

    // Fallback: if no shapes, use the bounding box.
    if (shapes.empty()) {
      if (bbox.area() > 0) {
        state.drc_rects.push_back({bbox, yellow_fill, "", /*filled=*/true});
      }
      continue;
    }

    for (const auto& shape : shapes) {
      if (std::holds_alternative<odb::Rect>(shape)) {
        state.drc_rects.push_back(
            {std::get<odb::Rect>(shape), yellow_fill, "", /*filled=*/true});
      } else if (std::holds_alternative<odb::Line>(shape)) {
        const odb::Line& line = std::get<odb::Line>(shape);
        state.drc_lines.push_back({line.pt0(), line.pt1(), yellow_line});
      } else if (std::holds_alternative<odb::Point>(shape)) {
        const odb::Point& pt = std::get<odb::Point>(shape);
        emitX(pt.x(), pt.y(), min_box / 2);
      } else if (std::holds_alternative<odb::Polygon>(shape)) {
        const odb::Polygon& poly = std::get<odb::Polygon>(shape);
        state.drc_rects.push_back(
            {poly.getEnclosingRect(), yellow_fill, "", /*filled=*/true});
      } else if (std::holds_alternative<odb::Cuboid>(shape)) {
        state.drc_rects.push_back(
            {std::get<odb::Cuboid>(shape).getEnclosingRect(),
             yellow_fill,
             "",
             /*filled=*/true});
      }
    }
  }
}

WebSocketResponse DRCHandler::handleDRCCategories(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;

  try {
    auto [block, chip] = getBlockAndChip();

    boost::json::object root;
    boost::json::array categories;
    for (odb::dbMarkerCategory* category : chip->getMarkerCategories()) {
      boost::json::object o;
      o["name"] = std::string(category->getName());
      o["count"] = category->getMarkerCount();
      const std::string desc = category->getDescription();
      if (!desc.empty()) {
        o["description"] = desc;
      }
      const std::string source = category->getSource();
      if (!source.empty()) {
        o["source"] = source;
      }
      categories.emplace_back(std::move(o));
    }
    root["categories"] = std::move(categories);
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    std::string err = std::string("drc_categories error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

// Recursive helper to serialize a marker category tree.
static boost::json::object serializeMarkerCategory(
    odb::dbMarkerCategory* category)
{
  boost::json::object o;
  o["name"] = std::string(category->getName());
  o["count"] = category->getMarkerCount();

  // Subcategories
  auto subcats = category->getMarkerCategories();
  if (subcats.begin() != subcats.end()) {
    boost::json::array arr;
    for (odb::dbMarkerCategory* sub : subcats) {
      arr.emplace_back(serializeMarkerCategory(sub));
    }
    o["subcategories"] = std::move(arr);
  }

  // Markers directly in this category
  auto markers = category->getMarkers();
  if (markers.begin() != markers.end()) {
    boost::json::array marker_arr;
    int idx = 1;
    for (odb::dbMarker* marker : markers) {
      boost::json::object m;
      m["id"] = static_cast<int>(marker->getId());
      m["index"] = idx++;
      m["name"] = marker->getName();
      m["visited"] = marker->isVisited();
      m["visible"] = marker->isVisible();
      m["waived"] = marker->isWaived();
      m["bbox"] = bboxArray(marker->getBBox());

      // Serialize individual shapes so the 3D viewer can highlight
      // the actual cuboids instead of the full bounding box.
      const auto& shapes = marker->getShapes();
      if (!shapes.empty()) {
        boost::json::array rects;
        for (const auto& shape : shapes) {
          if (std::holds_alternative<odb::Rect>(shape)) {
            const odb::Rect& r = std::get<odb::Rect>(shape);
            rects.emplace_back(
                boost::json::array{r.xMin(), r.yMin(), r.xMax(), r.yMax()});
          } else if (std::holds_alternative<odb::Polygon>(shape)) {
            const odb::Rect r
                = std::get<odb::Polygon>(shape).getEnclosingRect();
            rects.emplace_back(
                boost::json::array{r.xMin(), r.yMin(), r.xMax(), r.yMax()});
          } else if (std::holds_alternative<odb::Cuboid>(shape)) {
            const odb::Cuboid& c = std::get<odb::Cuboid>(shape);
            rects.emplace_back(boost::json::array{
                c.xMin(), c.yMin(), c.xMax(), c.yMax(), c.zMin(), c.zMax()});
          }
        }
        m["rects"] = std::move(rects);
      }

      if (odb::dbTechLayer* layer = marker->getTechLayer()) {
        m["layer"] = std::string(layer->getName());
      }

      const std::string comment = marker->getComment();
      if (!comment.empty()) {
        m["comment"] = comment;
      }

      auto sources = marker->getSources();
      if (!sources.empty()) {
        boost::json::array src_arr;
        for (odb::dbObject* src : sources) {
          boost::json::object s;
          switch (src->getObjectType()) {
            case odb::dbNetObj: {
              s["type"] = "Net";
              s["name"] = std::string(static_cast<odb::dbNet*>(src)->getName());
              break;
            }
            case odb::dbInstObj: {
              s["type"] = "Inst";
              s["name"]
                  = std::string(static_cast<odb::dbInst*>(src)->getName());
              break;
            }
            case odb::dbITermObj: {
              s["type"] = "ITerm";
              s["name"]
                  = std::string(static_cast<odb::dbITerm*>(src)->getName());
              break;
            }
            case odb::dbBTermObj: {
              s["type"] = "BTerm";
              s["name"]
                  = std::string(static_cast<odb::dbBTerm*>(src)->getName());
              break;
            }
            default:
              s["type"] = "Object";
              s["name"] = "unknown";
              break;
          }
          src_arr.emplace_back(std::move(s));
        }
        m["sources"] = std::move(src_arr);
      }
      marker_arr.emplace_back(std::move(m));
    }
    o["markers"] = std::move(marker_arr);
  }
  return o;
}

WebSocketResponse DRCHandler::handleDRCMarkers(const WebSocketRequest& req,
                                               SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;

  try {
    auto [block, chip] = getBlockAndChip();

    const std::string cat_name
        = std::string(req.json.at("category").as_string());

    // Clear all markers' visibility so highlights start off.
    // The user explicitly checks individual markers to see them.
    odb::dbMarkerCategory* category = nullptr;
    if (!cat_name.empty()) {
      category = chip->findMarkerCategory(cat_name.c_str());
      if (category) {
        for (odb::dbMarker* marker : category->getAllMarkers()) {
          marker->setVisible(false);
        }
      }
    }

    // Update active category and overlay (now empty since all invisible)
    {
      std::lock_guard<std::mutex> lock(state.drc_mutex);
      state.active_drc_category = cat_name;
      refreshDRCOverlay(state);
    }

    boost::json::object root;
    if (cat_name.empty()) {
      root["subcategories"] = boost::json::array{};
    } else {
      if (!category) {
        root["error"] = "Category not found: " + cat_name;
      } else {
        root = serializeMarkerCategory(category);
        root["total_count"] = category->getMarkerCount();
      }
    }
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    std::string err = std::string("drc_markers error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse DRCHandler::handleDRCLoadReport(const WebSocketRequest& req,
                                                  SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;

  try {
    auto [block, chip] = getBlockAndChip();

    const std::string path = std::string(req.json.at("path").as_string());
    if (path.empty()) {
      throw std::runtime_error("No file path provided");
    }

    odb::dbMarkerCategory* category = nullptr;
    if (path.ends_with(".rpt") || path.ends_with(".drc")) {
      category = odb::dbMarkerCategory::fromTR(chip, "DRC", path);
    } else if (path.ends_with(".json")) {
      auto categories = odb::dbMarkerCategory::fromJSON(chip, path);
      if (!categories.empty()) {
        category = *categories.begin();
      }
    } else {
      throw std::runtime_error("Unsupported file format: " + path);
    }

    boost::json::object root;
    if (category) {
      const std::string name = category->getName();
      root["ok"] = 1;
      root["category"] = name;
      root["count"] = category->getMarkerCount();

      // Auto-select the loaded category
      {
        std::lock_guard<std::mutex> lock(state.drc_mutex);
        state.active_drc_category = name;
        refreshDRCOverlay(state);
      }
    } else {
      root["ok"] = 0;
      root["error"] = "No violations found in report";
    }
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    std::string err = std::string("drc_load_report error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse DRCHandler::handleDRCUpdateMarker(const WebSocketRequest& req,
                                                    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;

  try {
    const int marker_id = static_cast<int>(req.json.at("marker_id").as_int64());
    const std::string field = std::string(req.json.at("field").as_string());
    const bool field_value = req.json.at("value").as_bool();
    auto [block, chip] = getBlockAndChip();

    odb::dbMarker* target = findMarkerById(state, chip, marker_id);
    if (!target) {
      throw std::runtime_error("Marker not found with id "
                               + std::to_string(marker_id));
    }

    if (field == "visited") {
      target->setVisited(field_value);
    } else if (field == "visible") {
      target->setVisible(field_value);
      std::lock_guard<std::mutex> lock(state.drc_mutex);
      refreshDRCOverlay(state);
    } else {
      throw std::runtime_error("Unknown field: " + field);
    }

    boost::json::object root;
    root["ok"] = 1;
    root["id"] = marker_id;
    root["field"] = field;
    root["value"] = field_value;
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    std::string err = std::string("drc_update_marker error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse DRCHandler::handleDRCUpdateCategoryVisibility(
    const WebSocketRequest& req,
    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;

  try {
    const std::string cat_name
        = std::string(req.json.at("category").as_string());
    const bool visible = req.json.at("visible").as_bool();
    auto [block, chip] = getBlockAndChip();

    std::lock_guard<std::mutex> lock(state.drc_mutex);
    odb::dbMarkerCategory* category
        = chip->findMarkerCategory(cat_name.c_str());
    if (!category) {
      throw std::runtime_error("Category not found: " + cat_name);
    }

    int count = 0;
    for (odb::dbMarker* marker : category->getAllMarkers()) {
      marker->setVisible(visible);
      ++count;
    }
    refreshDRCOverlay(state);

    boost::json::object root;
    root["ok"] = 1;
    root["category"] = cat_name;
    root["visible"] = visible;
    root["count"] = count;
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    std::string err
        = std::string("drc_update_category_visibility error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse DRCHandler::handleDRCHighlight(const WebSocketRequest& req,
                                                 SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;

  try {
    const int marker_id = static_cast<int>(req.json.at("marker_id").as_int64());
    const bool open_inspector = req.json.contains("open_inspector")
                                    ? req.json.at("open_inspector").as_bool()
                                    : false;
    auto [block, chip] = getBlockAndChip();

    odb::dbMarker* target = findMarkerById(state, chip, marker_id);

    boost::json::object root;
    if (target) {
      target->setVisited(true);
      odb::Rect bbox = target->getBBox();

      // When the client requests inspector navigation, promote the marker to
      // a canonical selectable so the existing `inspect` flow can populate
      // the Inspector panel.  Mirrors handleSelect's pattern (replace
      // selectables, set current_inspected, clear navigation history) so
      // back-navigation behaves the same as for instances/nets.
      gui::Selected sel;
      int marker_select_id = -1;
      std::vector<gui::Selected> new_selectables;
      if (open_inspector) {
        sel = gui::DescriptorRegistry::instance()->makeSelected(target);
        if (sel) {
          marker_select_id = storeSelectable(new_selectables, sel);
        }
      }

      {
        std::lock_guard<std::mutex> lock(state.selection_mutex);
        clearSelectionHighlights(state);
        if (sel) {
          state.hover_rects.clear();
          state.timing_rects.clear();
          state.timing_lines.clear();
          setSelectionHighlights(state, sel);
          runDeselectAction(state.current_inspected, sel);
          state.current_inspected = sel;
          state.navigation_history.clear();
        } else {
          state.highlight_rects.push_back(bbox);
          state.highlight_polys.clear();
          state.highlight_lines.clear();
        }
      }

      if (sel) {
        std::lock_guard<std::mutex> lock(state.selectables_mutex);
        state.selectables = std::move(new_selectables);
      }

      root["ok"] = 1;
      root["bbox"] = bboxArray(bbox);
      root["name"] = target->getName();
      root["visited"] = true;
      if (odb::dbTechLayer* layer = target->getTechLayer()) {
        root["layer"] = std::string(layer->getName());
      }
      if (marker_select_id >= 0) {
        root["select_id"] = marker_select_id;
      }
    } else {
      // Clear highlight if marker_id is -1 (deselect)
      if (marker_id == -1) {
        std::lock_guard<std::mutex> lock(state.selection_mutex);
        clearSelectionHighlights(state);
      }
      root["ok"] = 0;
    }
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    std::string err = std::string("drc_highlight error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

// ─── EditHandler: Global Connect + Insert Buffer ─────────────────────────────

namespace {
// Classify a net terminal as a driver, mirroring the Qt Insert Buffer dialog
// (dbDescriptors.cpp): output/inout iterms and input/inout/feedthru bterms
// drive the net.
bool itermIsDriver(odb::dbITerm* iterm)
{
  const odb::dbIoType io = iterm->getIoType();
  return io == odb::dbIoType::OUTPUT || io == odb::dbIoType::INOUT;
}
bool btermIsDriver(odb::dbBTerm* bterm)
{
  const odb::dbIoType io = bterm->getIoType();
  return io == odb::dbIoType::INPUT || io == odb::dbIoType::INOUT
         || io == odb::dbIoType::FEEDTHRU;
}

// Stable string id for a net pin, used to reference it across requests.
std::string itermId(odb::dbITerm* it)
{
  return "I:" + it->getInst()->getName() + "/" + it->getMTerm()->getName();
}
std::string btermId(odb::dbBTerm* bt)
{
  return "B:" + bt->getName();
}
}  // namespace

EditHandler::EditHandler(std::shared_ptr<TileGenerator> gen,
                         std::shared_ptr<TclEvaluator> tcl_eval)
    : gen_(std::move(gen)), tcl_eval_(std::move(tcl_eval))
{
}

void EditHandler::registerRequests(RequestDispatcher& d)
{
  d.add("global_connect_info",
        WebSocketRequest::kGlobalConnectInfo,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleGlobalConnectInfo(req);
        });
  d.add("global_connect_delete",
        WebSocketRequest::kGlobalConnectDelete,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleGlobalConnectDelete(req);
        });
  d.add("global_connect_apply",
        WebSocketRequest::kGlobalConnectApply,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleGlobalConnectApply(req);
        });
  d.add("buffer_info",
        WebSocketRequest::kBufferInfo,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleBufferInfo(req);
        });
  d.add("insert_buffer",
        WebSocketRequest::kInsertBuffer,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleInsertBuffer(req);
        });
}

WebSocketResponse EditHandler::handleGlobalConnectInfo(
    const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    std::lock_guard<std::mutex> db_lock(tcl_eval_->mutex);
    odb::dbBlock* block = gen_->getBlock();
    boost::json::object root;
    boost::json::array rules;
    boost::json::array nets;
    boost::json::array regions;
    if (block != nullptr) {
      for (auto* gc : block->getGlobalConnects()) {
        boost::json::object o;
        o["inst"] = gc->getInstPattern();
        o["pin"] = gc->getPinPattern();
        odb::dbNet* net = gc->getNet();
        o["net"] = net != nullptr ? net->getName() : std::string();
        odb::dbRegion* region = gc->getRegion();
        o["region"] = region != nullptr ? std::string(region->getName())
                                        : std::string();
        rules.emplace_back(std::move(o));
      }
      // Net combo lists only special (power/ground) nets, like the Qt dialog.
      for (auto* net : block->getNets()) {
        if (net->isSpecial()) {
          nets.emplace_back(net->getName());
        }
      }
      for (auto* region : block->getRegions()) {
        regions.emplace_back(std::string(region->getName()));
      }
    }
    root["rules"] = std::move(rules);
    root["nets"] = std::move(nets);
    root["regions"] = std::move(regions);
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err
        = std::string("global_connect_info error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse EditHandler::handleGlobalConnectDelete(
    const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    std::lock_guard<std::mutex> db_lock(tcl_eval_->mutex);
    odb::dbBlock* block = gen_->getBlock();
    // Identify the rule by its (inst, pin, net, region) fields rather than a
    // positional index, so a concurrent add/delete can't shift indices and
    // make us destroy the wrong rule.
    const std::string inst = std::string(req.json.at("inst").as_string());
    const std::string pin = std::string(req.json.at("pin").as_string());
    const std::string net = std::string(req.json.at("net").as_string());
    const std::string region = std::string(req.json.at("region").as_string());
    boost::json::object root;
    root["ok"] = 0;
    if (block != nullptr) {
      for (auto* gc : block->getGlobalConnects()) {
        odb::dbNet* gc_net = gc->getNet();
        odb::dbRegion* gc_region = gc->getRegion();
        const std::string gc_net_name
            = gc_net != nullptr ? gc_net->getName() : std::string();
        const std::string gc_region_name
            = gc_region != nullptr ? std::string(gc_region->getName())
                                   : std::string();
        if (gc->getInstPattern() == inst && gc->getPinPattern() == pin
            && gc_net_name == net && gc_region_name == region) {
          odb::dbGlobalConnect::destroy(gc);
          root["ok"] = 1;
          break;
        }
      }
    }
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err
        = std::string("global_connect_delete error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse EditHandler::handleGlobalConnectApply(
    const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    std::lock_guard<std::mutex> db_lock(tcl_eval_->mutex);
    odb::dbBlock* block = gen_->getBlock();
    const bool force
        = req.json.contains("force") && req.json.at("force").as_bool();
    boost::json::object root;
    // Guard the empty case ourselves so the client shows a friendly message
    // instead of the raw ODB-0378 "Global connections are not set up." warning.
    if (block == nullptr || block->getGlobalConnects().empty()) {
      root["ok"] = true;
      root["had_rules"] = false;
      root["connected"] = 0;
    } else {
      const int connected = block->globalConnect(force, /*verbose=*/false);
      root["ok"] = true;
      root["had_rules"] = true;
      root["connected"] = connected;
    }
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err
        = std::string("global_connect_apply error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse EditHandler::handleBufferInfo(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    std::lock_guard<std::mutex> db_lock(tcl_eval_->mutex);
    odb::dbBlock* block = gen_->getBlock();
    const std::string net_name = std::string(req.json.at("net").as_string());
    boost::json::object root;
    boost::json::array drivers;
    boost::json::array loads;
    odb::dbNet* net = block ? block->findNet(net_name.c_str()) : nullptr;
    if (net == nullptr) {
      root["can_buffer"] = false;
      root["error"] = "Net not found: " + net_name;
      writePayload(resp, root);
      return resp;
    }
    for (auto* iterm : net->getITerms()) {
      boost::json::object p;
      const std::string id = itermId(iterm);
      const bool drv = itermIsDriver(iterm);
      p["id"] = id;
      p["label"] = id.substr(2) + (drv ? " (Driver)" : " (Load)");
      (drv ? drivers : loads).emplace_back(std::move(p));
    }
    for (auto* bterm : net->getBTerms()) {
      boost::json::object p;
      p["id"] = btermId(bterm);
      const bool drv = btermIsDriver(bterm);
      p["label"] = bterm->getName() + (drv ? " (Driver Port)" : " (Load Port)");
      (drv ? drivers : loads).emplace_back(std::move(p));
    }

    // Buffer masters = db masters mapping to a Liberty buffer cell.
    boost::json::array masters;
    sta::dbSta* sta = gen_->getSta();
    sta::dbNetwork* network = sta != nullptr ? sta->getDbNetwork() : nullptr;
    if (network != nullptr) {
      std::vector<std::string> names;
      for (auto* lib : gen_->getDb()->getLibs()) {
        for (auto* master : lib->getMasters()) {
          sta::LibertyCell* cell
              = network->libertyCell(network->dbToSta(master));
          if (cell != nullptr && cell->isBuffer()) {
            names.push_back(master->getName());
          }
        }
      }
      std::sort(names.begin(), names.end());
      for (const auto& n : names) {
        masters.emplace_back(n);
      }
    }

    root["can_buffer"] = drivers.size() <= 1;
    root["drivers"] = std::move(drivers);
    root["loads"] = std::move(loads);
    root["masters"] = std::move(masters);
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("buffer_info error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse EditHandler::handleInsertBuffer(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    // Serialize this DB mutation against the Tcl write path and other edits.
    std::lock_guard<std::mutex> db_lock(tcl_eval_->mutex);
    odb::dbBlock* block = gen_->getBlock();
    const std::string net_name = std::string(req.json.at("net").as_string());
    const std::string mode = std::string(req.json.at("mode").as_string());
    const std::string master_name
        = std::string(req.json.at("master").as_string());
    const std::string buf_name
        = req.json.contains("buf_name")
              ? std::string(req.json.at("buf_name").as_string())
              : "";
    const std::string new_net_name
        = req.json.contains("net_name")
              ? std::string(req.json.at("net_name").as_string())
              : "";

    odb::dbNet* net = block ? block->findNet(net_name.c_str()) : nullptr;
    if (net == nullptr) {
      throw std::runtime_error("Net not found: " + net_name);
    }

    // Resolve the master by name (odb searches across all libs).
    odb::dbMaster* master = gen_->getDb()->findMaster(master_name.c_str());
    if (master == nullptr) {
      throw std::runtime_error("Buffer master not found: " + master_name);
    }

    // Build id → terminal map for this net (same ids as buffer_info).
    std::map<std::string, odb::dbObject*> pin_map;
    for (auto* iterm : net->getITerms()) {
      pin_map[itermId(iterm)] = iterm;
    }
    for (auto* bterm : net->getBTerms()) {
      pin_map[btermId(bterm)] = bterm;
    }

    const auto& pins = req.json.at("pins").as_array();
    const char* buf_base
        = buf_name.empty() ? kDefaultBufBaseName : buf_name.c_str();
    const char* net_base
        = new_net_name.empty() ? kDefaultNetBaseName : new_net_name.c_str();

    odb::dbInst* inst = nullptr;
    if (mode == "driver") {
      if (pins.empty()) {
        throw std::runtime_error("No driver pin selected.");
      }
      auto it = pin_map.find(std::string(pins.front().as_string()));
      if (it == pin_map.end()) {
        throw std::runtime_error("Driver pin not on net.");
      }
      inst = net->insertBufferAfterDriver(it->second,
                                          master,
                                          nullptr,
                                          buf_base,
                                          net_base,
                                          odb::dbNameUniquifyType::IF_NEEDED);
    } else {
      std::vector<odb::dbObject*> loads;
      for (const auto& pin : pins) {
        auto it = pin_map.find(std::string(pin.as_string()));
        if (it != pin_map.end()) {
          loads.push_back(it->second);
        }
      }
      if (loads.empty()) {
        throw std::runtime_error("No load pins selected.");
      }
      inst = net->insertBufferBeforeLoads(loads,
                                          master,
                                          nullptr,
                                          buf_base,
                                          net_base,
                                          odb::dbNameUniquifyType::IF_NEEDED);
    }

    // The buffer is left PLACED at the pin (insertBuffer*'s default) so it
    // renders immediately — the web tile index only draws placed instances
    // (search.cpp).  PLACED does not pin it: detailed_placement still
    // legalizes/moves it (only LOCKED/FIRM are fixed).

    boost::json::object root;
    root["ok"] = inst != nullptr;
    root["inst"] = inst != nullptr ? inst->getName() : std::string();
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("insert_buffer error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

}  // namespace web
