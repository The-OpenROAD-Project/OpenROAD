// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2026, The OpenROAD Authors

#include <algorithm>
#include <any>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "absl/synchronization/mutex.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gui/gui.h"
#include "gui/heatMap.h"
#include "heatMapPinDensity.h"
#include "heatMapPlacementDensity.h"
#include "odb/db.h"
#include "sta/PowerClass.hh"
#include "utl/Logger.h"

namespace gui {

// Heatmap / Spectrum colors
// https://ai.googleblog.com/2019/08/turbo-improved-rainbow-colormap-for.html
// https://gist.github.com/mikhailov-work/6a308c20e494d9e0ccc29036b28faa7a
const unsigned char SpectrumGenerator::kSpectrum[256][3]
    = {{48, 18, 59},   {50, 21, 67},   {51, 24, 74},    {52, 27, 81},
       {53, 30, 88},   {54, 33, 95},   {55, 36, 102},   {56, 39, 109},
       {57, 42, 115},  {58, 45, 121},  {59, 47, 128},   {60, 50, 134},
       {61, 53, 139},  {62, 56, 145},  {63, 59, 151},   {63, 62, 156},
       {64, 64, 162},  {65, 67, 167},  {65, 70, 172},   {66, 73, 177},
       {66, 75, 181},  {67, 78, 186},  {68, 81, 191},   {68, 84, 195},
       {68, 86, 199},  {69, 89, 203},  {69, 92, 207},   {69, 94, 211},
       {70, 97, 214},  {70, 100, 218}, {70, 102, 221},  {70, 105, 224},
       {70, 107, 227}, {71, 110, 230}, {71, 113, 233},  {71, 115, 235},
       {71, 118, 238}, {71, 120, 240}, {71, 123, 242},  {70, 125, 244},
       {70, 128, 246}, {70, 130, 248}, {70, 133, 250},  {70, 135, 251},
       {69, 138, 252}, {69, 140, 253}, {68, 143, 254},  {67, 145, 254},
       {66, 148, 255}, {65, 150, 255}, {64, 153, 255},  {62, 155, 254},
       {61, 158, 254}, {59, 160, 253}, {58, 163, 252},  {56, 165, 251},
       {55, 168, 250}, {53, 171, 248}, {51, 173, 247},  {49, 175, 245},
       {47, 178, 244}, {46, 180, 242}, {44, 183, 240},  {42, 185, 238},
       {40, 188, 235}, {39, 190, 233}, {37, 192, 231},  {35, 195, 228},
       {34, 197, 226}, {32, 199, 223}, {31, 201, 221},  {30, 203, 218},
       {28, 205, 216}, {27, 208, 213}, {26, 210, 210},  {26, 212, 208},
       {25, 213, 205}, {24, 215, 202}, {24, 217, 200},  {24, 219, 197},
       {24, 221, 194}, {24, 222, 192}, {24, 224, 189},  {25, 226, 187},
       {25, 227, 185}, {26, 228, 182}, {28, 230, 180},  {29, 231, 178},
       {31, 233, 175}, {32, 234, 172}, {34, 235, 170},  {37, 236, 167},
       {39, 238, 164}, {42, 239, 161}, {44, 240, 158},  {47, 241, 155},
       {50, 242, 152}, {53, 243, 148}, {56, 244, 145},  {60, 245, 142},
       {63, 246, 138}, {67, 247, 135}, {70, 248, 132},  {74, 248, 128},
       {78, 249, 125}, {82, 250, 122}, {85, 250, 118},  {89, 251, 115},
       {93, 252, 111}, {97, 252, 108}, {101, 253, 105}, {105, 253, 102},
       {109, 254, 98}, {113, 254, 95}, {117, 254, 92},  {121, 254, 89},
       {125, 255, 86}, {128, 255, 83}, {132, 255, 81},  {136, 255, 78},
       {139, 255, 75}, {143, 255, 73}, {146, 255, 71},  {150, 254, 68},
       {153, 254, 66}, {156, 254, 64}, {159, 253, 63},  {161, 253, 61},
       {164, 252, 60}, {167, 252, 58}, {169, 251, 57},  {172, 251, 56},
       {175, 250, 55}, {177, 249, 54}, {180, 248, 54},  {183, 247, 53},
       {185, 246, 53}, {188, 245, 52}, {190, 244, 52},  {193, 243, 52},
       {195, 241, 52}, {198, 240, 52}, {200, 239, 52},  {203, 237, 52},
       {205, 236, 52}, {208, 234, 52}, {210, 233, 53},  {212, 231, 53},
       {215, 229, 53}, {217, 228, 54}, {219, 226, 54},  {221, 224, 55},
       {223, 223, 55}, {225, 221, 55}, {227, 219, 56},  {229, 217, 56},
       {231, 215, 57}, {233, 213, 57}, {235, 211, 57},  {236, 209, 58},
       {238, 207, 58}, {239, 205, 58}, {241, 203, 58},  {242, 201, 58},
       {244, 199, 58}, {245, 197, 58}, {246, 195, 58},  {247, 193, 58},
       {248, 190, 57}, {249, 188, 57}, {250, 186, 57},  {251, 184, 56},
       {251, 182, 55}, {252, 179, 54}, {252, 177, 54},  {253, 174, 53},
       {253, 172, 52}, {254, 169, 51}, {254, 167, 50},  {254, 164, 49},
       {254, 161, 48}, {254, 158, 47}, {254, 155, 45},  {254, 153, 44},
       {254, 150, 43}, {254, 147, 42}, {254, 144, 41},  {253, 141, 39},
       {253, 138, 38}, {252, 135, 37}, {252, 132, 35},  {251, 129, 34},
       {251, 126, 33}, {250, 123, 31}, {249, 120, 30},  {249, 117, 29},
       {248, 114, 28}, {247, 111, 26}, {246, 108, 25},  {245, 105, 24},
       {244, 102, 23}, {243, 99, 21},  {242, 96, 20},   {241, 93, 19},
       {240, 91, 18},  {239, 88, 17},  {237, 85, 16},   {236, 83, 15},
       {235, 80, 14},  {234, 78, 13},  {232, 75, 12},   {231, 73, 12},
       {229, 71, 11},  {228, 69, 10},  {226, 67, 10},   {225, 65, 9},
       {223, 63, 8},   {221, 61, 8},   {220, 59, 7},    {218, 57, 7},
       {216, 55, 6},   {214, 53, 6},   {212, 51, 5},    {210, 49, 5},
       {208, 47, 5},   {206, 45, 4},   {204, 43, 4},    {202, 42, 4},
       {200, 40, 3},   {197, 38, 3},   {195, 37, 3},    {193, 35, 2},
       {190, 33, 2},   {188, 32, 2},   {185, 30, 2},    {183, 29, 2},
       {180, 27, 1},   {178, 26, 1},   {175, 24, 1},    {172, 23, 1},
       {169, 22, 1},   {167, 20, 1},   {164, 19, 1},    {161, 18, 1},
       {158, 16, 1},   {155, 15, 1},   {152, 14, 1},    {149, 13, 1},
       {146, 11, 1},   {142, 10, 1},   {139, 9, 2},     {136, 8, 2},
       {133, 7, 2},    {129, 6, 2},    {126, 5, 2},     {122, 4, 3}};

SpectrumGenerator::SpectrumGenerator(double max_value) : scale_(1.0 / max_value)
{
}

int SpectrumGenerator::getColorCount() const
{
  return 256;
}

Painter::Color SpectrumGenerator::getColor(double value, int alpha) const
{
  const int max_index = getColorCount() - 1;
  int index = std::round(scale_ * value * max_index);
  if (index < 0) {
    index = 0;
  } else if (index > max_index) {
    index = max_index;
  }

  return Painter::Color(
      kSpectrum[index][0], kSpectrum[index][1], kSpectrum[index][2], alpha);
}

namespace {

std::mutex& heatMapSourceMutex()
{
  static std::mutex mutex;
  return mutex;
}

std::vector<HeatMapSourceHandle>& heatMapSources()
{
  static std::vector<HeatMapSourceHandle> sources;
  return sources;
}

std::string formatFixed(double value, int digits)
{
  std::ostringstream out;
  out << std::fixed << std::setprecision(digits) << value;
  return out.str();
}

}  // namespace

HeatMapDataSource::HeatMapDataSource(utl::Logger* logger,
                                     const std::string& name,
                                     const std::string& short_name,
                                     const std::string& settings_group)
    : name_(name),
      short_name_(short_name),
      settings_group_(settings_group),
      destroy_map_(true),
      use_dbu_(false),
      populated_(false),
      colors_correct_(false),
      issue_redraw_(true),
      chip_(nullptr),
      logger_(logger),
      grid_x_size_(10.0),
      grid_y_size_(10.0),
      display_range_min_(0.0),
      display_range_max_(100.0),
      draw_below_min_display_range_(false),
      draw_above_max_display_range_(true),
      color_alpha_(150),
      log_scale_(false),
      reverse_log_(false),
      show_numbers_(false),
      show_legend_(false),
      use_selected_only_(false),
      color_generator_(SpectrumGenerator(100.0))
{
  clearMap();
  updateMapColors();
}

HeatMapDataSource::~HeatMapDataSource()
{
  if (unregister_callback_) {
    unregister_callback_(this);
  }
}

void HeatMapDataSource::dumpToFile(const std::string& file)
{
  ensureMap();

  if (!isPopulated()) {
    logger_->error(utl::GUI, 72, "\"{}\" is not populated with data.", name_);
  }

  std::ofstream csv(file);
  if (!csv.is_open()) {
    logger_->error(utl::GUI, 73, "Unable to open {}", file);
  }

  const double dbu_to_micron = getDbuPerMicron();

  csv << "x0,y0,x1,y1,value (" << getValueUnits() << ")\n";
  for (const auto& map_col : map_) {
    for (const auto& map_value : map_col) {
      if (!map_value->has_value) {
        continue;
      }
      const odb::Rect& box_rect = map_value->rect;
      const double scaled_value = convertPercentToValue(map_value->value);

      csv << std::defaultfloat << std::setprecision(4);
      csv << box_rect.xMin() / dbu_to_micron << ",";
      csv << box_rect.yMin() / dbu_to_micron << ",";
      csv << box_rect.xMax() / dbu_to_micron << ",";
      csv << box_rect.yMax() / dbu_to_micron << ",";
      csv << std::scientific << std::setprecision(6);
      csv << scaled_value << '\n';
    }
  }
}

void HeatMapDataSource::redraw()
{
  if (issue_redraw_ && redraw_callback_) {
    redraw_callback_();
  }
}

void HeatMapDataSource::showSetup()
{
  if (setup_callback_) {
    setup_callback_();
  }
}

void HeatMapDataSource::setRedrawCallback(std::function<void()> callback)
{
  redraw_callback_ = std::move(callback);
}

void HeatMapDataSource::setSetupCallback(std::function<void()> callback)
{
  setup_callback_ = std::move(callback);
}

void HeatMapDataSource::setUnregisterCallback(
    std::function<void(HeatMapDataSource*)> callback)
{
  unregister_callback_ = std::move(callback);
}

void HeatMapDataSource::setColorAlpha(int alpha)
{
  color_alpha_
      = boundValue<int>(alpha, getColorAlphaMinimum(), getColorAlphaMaximum());
  updateMapColors();
  redraw();
}

void HeatMapDataSource::setDisplayRange(double min, double max)
{
  if (max < min) {
    std::swap(min, max);
  }

  display_range_min_ = boundValue<double>(
      min, getDisplayRangeMinimumValue(), getDisplayRangeMaximumValue());
  display_range_max_ = boundValue<double>(
      max, getDisplayRangeMinimumValue(), getDisplayRangeMaximumValue());

  updateMapColors();
  redraw();
}

void HeatMapDataSource::setDrawBelowRangeMin(bool show)
{
  draw_below_min_display_range_ = show;
  redraw();
}

void HeatMapDataSource::setDrawAboveRangeMax(bool show)
{
  draw_above_max_display_range_ = show;
  redraw();
}

void HeatMapDataSource::setGridSizes(double x, double y)
{
  bool changed = false;
  if (grid_x_size_ != x) {
    grid_x_size_ = boundValue<double>(
        x, getGridSizeMinimumValue(), getGridSizeMaximumValue());
    changed = true;
  }
  if (grid_y_size_ != y) {
    grid_y_size_ = boundValue<double>(
        y, getGridSizeMinimumValue(), getGridSizeMaximumValue());
    changed = true;
  }

  if (changed) {
    destroyMap();
  }
}

void HeatMapDataSource::setLogScale(bool scale)
{
  log_scale_ = scale;
  updateMapColors();
  redraw();
}

void HeatMapDataSource::setReverseLogScale(bool reverse)
{
  reverse_log_ = reverse;
  updateMapColors();
  redraw();
}

void HeatMapDataSource::setShowNumbers(bool numbers)
{
  show_numbers_ = numbers;
  redraw();
}

void HeatMapDataSource::setShowLegend(bool legend)
{
  show_legend_ = legend;
  redraw();
}

Painter::Color HeatMapDataSource::getColor(double value) const
{
  auto find_val = std::ranges::find_if(
      color_lower_bounds_,
      [value](const double other) { return other >= value; });
  const double color_index
      = std::distance(color_lower_bounds_.begin(), find_val);
  return color_generator_.getColor(
      100.0 * color_index / color_generator_.getColorCount(), color_alpha_);
}

std::string HeatMapDataSource::formatValue(double value, bool legend) const
{
  std::string text = formatFixed(value, 2);
  if (legend) {
    text += "%";
  }
  return text;
}

void HeatMapDataSource::addBooleanSetting(
    const std::string& name,
    const std::string& label,
    const std::function<bool()>& getter,
    const std::function<void(bool)>& setter)
{
  settings_.emplace_back(MapSettingBoolean{name, label, getter, setter});
}

void HeatMapDataSource::addMultipleChoiceSetting(
    const std::string& name,
    const std::string& label,
    const std::function<std::vector<std::string>()>& choices,
    const std::function<std::string()>& getter,
    const std::function<void(std::string)>& setter)
{
  settings_.emplace_back(
      MapSettingMultiChoice{name, label, choices, getter, setter});
}

Renderer::Settings HeatMapDataSource::getSettings() const
{
  Renderer::Settings settings{{"DisplayMin", display_range_min_},
                              {"DisplayMax", display_range_max_},
                              {"GridX", grid_x_size_},
                              {"GridY", grid_y_size_},
                              {"Alpha", color_alpha_},
                              {"LogScale", log_scale_},
                              {"ReverseLog", reverse_log_},
                              {"ShowNumbers", show_numbers_},
                              {"ShowLegend", show_legend_},
                              {"UseSelectedOnly", use_selected_only_},
                              {"ShowMin", draw_below_min_display_range_},
                              {"ShowMax", draw_above_max_display_range_}};

  for (const auto& setting : settings_) {
    if (std::holds_alternative<MapSettingBoolean>(setting)) {
      const auto& set = std::get<MapSettingBoolean>(setting);
      settings[set.name] = set.getter();
    } else if (std::holds_alternative<MapSettingMultiChoice>(setting)) {
      const auto& set = std::get<MapSettingMultiChoice>(setting);
      settings[set.name] = set.getter();
    }
  }

  return settings;
}

void HeatMapDataSource::setSettings(const Renderer::Settings& settings)
{
  Renderer::setSetting<double>(settings, "DisplayMin", display_range_min_);
  Renderer::setSetting<double>(settings, "DisplayMax", display_range_max_);
  Renderer::setSetting<double>(settings, "GridX", grid_x_size_);
  Renderer::setSetting<double>(settings, "GridY", grid_y_size_);
  Renderer::setSetting<int>(settings, "Alpha", color_alpha_);
  Renderer::setSetting<bool>(settings, "LogScale", log_scale_);
  Renderer::setSetting<bool>(settings, "ReverseLog", reverse_log_);
  Renderer::setSetting<bool>(settings, "ShowNumbers", show_numbers_);
  Renderer::setSetting<bool>(settings, "ShowLegend", show_legend_);
  Renderer::setSetting<bool>(settings, "UseSelectedOnly", use_selected_only_);
  Renderer::setSetting<bool>(
      settings, "ShowMin", draw_below_min_display_range_);
  Renderer::setSetting<bool>(
      settings, "ShowMax", draw_above_max_display_range_);

  for (const auto& setting : settings_) {
    if (std::holds_alternative<MapSettingBoolean>(setting)) {
      const auto& set = std::get<MapSettingBoolean>(setting);
      bool temp_value = set.getter();
      Renderer::setSetting<bool>(settings, set.name, temp_value);
      set.setter(temp_value);
    } else if (std::holds_alternative<MapSettingMultiChoice>(setting)) {
      const auto& set = std::get<MapSettingMultiChoice>(setting);
      std::string temp_value = set.getter();
      Renderer::setSetting<std::string>(settings, set.name, temp_value);
      set.setter(temp_value);
    }
  }

  setDisplayRange(display_range_min_, display_range_max_);
  setGridSizes(grid_x_size_, grid_y_size_);
  setColorAlpha(color_alpha_);
}

std::set<odb::dbInst*> HeatMapDataSource::getSelectedInsts() const
{
  std::set<odb::dbInst*> selected_insts;
#ifdef ENABLE_QT
  if (!useSelectedOnly() || !gui::Gui::enabled()) {
    return selected_insts;
  }
  for (const gui::Selected& item : gui::Gui::get()->selection()) {
    if (item.isInst()) {
      selected_insts.insert(std::any_cast<odb::dbInst*>(item.getObject()));
    }
  }
#endif
  return selected_insts;
}

HeatMapDataSource::MapView HeatMapDataSource::getMapView(
    const odb::Rect& bounds)
{
  const auto x_low_find = std::ranges::lower_bound(map_x_grid_, bounds.xMin());
  const auto x_high_find
      = std::upper_bound(x_low_find, map_x_grid_.end(), bounds.xMax());
  const auto y_low_find = std::ranges::lower_bound(map_y_grid_, bounds.yMin());
  const auto y_high_find
      = std::upper_bound(y_low_find, map_y_grid_.end(), bounds.yMax());

  const int shape_x = static_cast<int>(map_.shape()[0]);
  const int shape_y = static_cast<int>(map_.shape()[1]);

  const int x_low = std::max(
      static_cast<int>(std::distance(map_x_grid_.begin(), x_low_find)) - 1, 0);
  const int x_high = std::min(
      static_cast<int>(std::distance(map_x_grid_.begin(), x_high_find)),
      shape_x);
  const int y_low = std::max(
      static_cast<int>(std::distance(map_y_grid_.begin(), y_low_find)) - 1, 0);
  const int y_high = std::min(
      static_cast<int>(std::distance(map_y_grid_.begin(), y_high_find)),
      shape_y);

  return map_[boost::indices[Map::index_range(x_low, x_high)]
                            [Map::index_range(y_low, y_high)]];
}

std::vector<HeatMapDataSource::MapColor> HeatMapDataSource::getVisibleMap(
    const odb::Rect& bounds,
    const double pixels_per_dbu,
    const double min_pixels_per_bin)
{
  ensureMap();

  if (!isPopulated()) {
    return {};
  }

  const double min_value = getRealRangeMinimumValue();
  const double max_value = getRealRangeMaximumValue();
  const bool show_mins = getDrawBelowRangeMin();
  const bool show_maxs = getDrawAboveRangeMax();

  const double min_dbu = (pixels_per_dbu > 0.0 && min_pixels_per_bin > 0.0)
                             ? min_pixels_per_bin / pixels_per_dbu
                             : 0.0;

  const double dbu_per_micron = getDbuPerMicron();
  const int x_scale
      = min_dbu <= 0.0
            ? 1
            : std::max(1,
                       static_cast<int>(std::ceil(
                           min_dbu / (getGridXSize() * dbu_per_micron))));
  const int y_scale
      = min_dbu <= 0.0
            ? 1
            : std::max(1,
                       static_cast<int>(std::ceil(
                           min_dbu / (getGridYSize() * dbu_per_micron))));

  const HeatMapDataSource::MapView map_view = getMapView(bounds);
  const int x_size = map_view.shape()[0];
  const int y_size = map_view.shape()[1];

  std::vector<MapColor> visible_map;
  for (int x = 0; x < x_size; x += x_scale) {
    for (int y = 0; y < y_size; y += y_scale) {
      MapColor draw_pt;
      draw_pt.rect.mergeInit();
      draw_pt.has_value = false;
      draw_pt.value = std::numeric_limits<double>::lowest();

      for (int x_sub = 0; x_sub < x_scale; x_sub++) {
        const int x_idx = x + x_sub;
        if (x_idx >= x_size) {
          continue;
        }

        for (int y_sub = 0; y_sub < y_scale; y_sub++) {
          const int y_idx = y + y_sub;
          if (y_idx >= y_size) {
            continue;
          }

          const auto& map_pt = map_view[x_idx][y_idx];
          draw_pt.rect.merge(map_pt->rect);
          if (!map_pt->has_value) {
            continue;
          }

          if (draw_pt.value < map_pt->value) {
            draw_pt.has_value = true;
            draw_pt.value = map_pt->value;
            draw_pt.color = map_pt->color;
          }
        }
      }

      if (!draw_pt.has_value) {
        continue;
      }
      if (!show_mins && draw_pt.value < min_value) {
        continue;
      }
      if (!show_maxs && draw_pt.value > max_value) {
        continue;
      }

      visible_map.push_back(draw_pt);
    }
  }

  return visible_map;
}

void HeatMapDataSource::addToMap(const odb::Rect& region, double value)
{
  for (const auto& map_col : getMapView(region)) {
    for (const auto& map_pt : map_col) {
      if (map_pt == nullptr) {
        continue;
      }

      odb::Rect intersection;
      map_pt->rect.intersection(region, intersection);

      const double intersect_area = intersection.area();
      const double value_area = region.area();
      const double region_area = map_pt->rect.area();

      combineMapData(map_pt->has_value,
                     map_pt->value,
                     value,
                     value_area,
                     intersect_area,
                     region_area);
      map_pt->has_value = true;

      markColorsInvalid();
    }
  }
}

odb::Rect HeatMapDataSource::getBounds() const
{
  if (getBlock() == nullptr) {
    return getChip()->getBBox();
  }
  return getBlock()->getDieArea();
}

odb::dbBlock* HeatMapDataSource::getBlock() const
{
  return chip_ != nullptr ? chip_->getBlock() : nullptr;
}

double HeatMapDataSource::getDbuPerMicron() const
{
  return chip_->getDb()->getDbuPerMicron();
}

void HeatMapDataSource::clearMap()
{
  map_.resize(boost::extents[1][1]);
  map_[0][0] = nullptr;
  populated_ = false;
}

bool HeatMapDataSource::setupMap()
{
  if (getChip() == nullptr || getBounds().area() == 0) {
    return false;
  }

  populateXYGrid();

  const size_t x_grid_size = map_x_grid_.size() - 1;
  const size_t y_grid_size = map_y_grid_.size() - 1;

  debugPrint(logger_,
             utl::GUI,
             "HeatMap",
             1,
             "{} - Generating {}x{} map",
             name_,
             x_grid_size,
             y_grid_size);
  map_.resize(boost::extents[x_grid_size][y_grid_size]);

  const Painter::Color default_color = getColor(0);
  for (size_t x = 0; x < x_grid_size; x++) {
    const int x_min = map_x_grid_[x];
    const int x_max = map_x_grid_[x + 1];

    for (size_t y = 0; y < y_grid_size; y++) {
      const int y_min = map_y_grid_[y];
      const int y_max = map_y_grid_[y + 1];

      auto map_pt = std::make_shared<MapColor>();
      map_pt->rect = odb::Rect(x_min, y_min, x_max, y_max);
      map_pt->has_value = false;
      map_pt->value = 0.0;
      map_pt->color = default_color;

      map_[x][y] = std::move(map_pt);
    }
  }

  return true;
}

void HeatMapDataSource::populateXYGrid()
{
  const int dx = getGridXSize() * getDbuPerMicron();
  const int dy = getGridYSize() * getDbuPerMicron();

  const odb::Rect bounds = getBounds();

  const int x_grid = std::ceil(bounds.dx() / static_cast<double>(dx));
  const int y_grid = std::ceil(bounds.dy() / static_cast<double>(dy));

  std::vector<int> x_grid_set, y_grid_set;
  for (int x = 0; x < x_grid; x++) {
    const int x_min = bounds.xMin() + x * dx;
    const int x_max = std::min(x_min + dx, bounds.xMax());
    if (x == 0) {
      x_grid_set.push_back(x_min);
    }
    x_grid_set.push_back(x_max);
  }
  for (int y = 0; y < y_grid; y++) {
    const int y_min = bounds.yMin() + y * dy;
    const int y_max = std::min(y_min + dy, bounds.yMax());
    if (y == 0) {
      y_grid_set.push_back(y_min);
    }
    y_grid_set.push_back(y_max);
  }

  setXYMapGrid(x_grid_set, y_grid_set);
}

void HeatMapDataSource::setXYMapGrid(const std::vector<int>& x_grid,
                                     const std::vector<int>& y_grid)
{
  const std::set<int> x_grid_set(x_grid.begin(), x_grid.end());
  const std::set<int> y_grid_set(y_grid.begin(), y_grid.end());

  map_x_grid_.clear();
  map_y_grid_.clear();

  map_x_grid_.insert(map_x_grid_.end(), x_grid_set.begin(), x_grid_set.end());
  map_y_grid_.insert(map_y_grid_.end(), y_grid_set.begin(), y_grid_set.end());
}

void HeatMapDataSource::destroyMap()
{
  if (destroy_map_) {
    return;
  }

  debugPrint(
      logger_, utl::GUI, "HeatMap", 1, "{} - destroy map requested", name_);

  destroy_map_ = true;
  redraw();
}

bool HeatMapDataSource::hasData() const
{
  if (!populated_) {
    return false;
  }

  for (const auto& map_col : map_) {
    for (const auto& map_pt : map_col) {
      if (map_pt->has_value) {
        return true;
      }
    }
  }

  return false;
}

void HeatMapDataSource::ensureMap()
{
  absl::MutexLock lock(&ensure_mutex_);

  if (destroy_map_) {
    debugPrint(logger_, utl::GUI, "HeatMap", 1, "{} - Destroying map", name_);
    clearMap();
    destroy_map_ = false;
  }

  const bool build_map = map_[0][0] == nullptr;
  if (build_map) {
    debugPrint(logger_, utl::GUI, "HeatMap", 1, "{} - Setting up map", name_);
    if (!setupMap()) {
      debugPrint(
          logger_, utl::GUI, "HeatMap", 1, "{} - No map available", name_);
      return;
    }
  }

  if (build_map || !isPopulated()) {
    debugPrint(logger_, utl::GUI, "HeatMap", 1, "{} - Populating map", name_);
    populated_ = populateMap();

    if (isPopulated()) {
      debugPrint(
          logger_, utl::GUI, "HeatMap", 1, "{} - Correcting map scale", name_);
      correctMapScale(map_);
    }
  }

  if (!colors_correct_ && isPopulated()) {
    debugPrint(
        logger_, utl::GUI, "HeatMap", 1, "{} - Assigning map colors", name_);
    assignMapColors();
  }
}

void HeatMapDataSource::updateMapColors()
{
  const int color_count = color_generator_.getColorCount();
  color_lower_bounds_.clear();
  color_lower_bounds_.resize(color_count + 1);
  if (log_scale_) {
    double range = display_range_max_;
    if (display_range_min_ != 0.0) {
      range = display_range_max_ / display_range_min_;
    }

    const double step = std::pow(range, 1.0 / color_count);

    for (int i = 0; i <= color_count; i++) {
      double start = display_range_max_ / std::pow(step, i);
      if (i == color_generator_.getColorCount()) {
        start = display_range_min_;
      }
      color_lower_bounds_[i] = start;
    }

    if (reverse_log_) {
      for (auto& lower_bound : color_lower_bounds_) {
        lower_bound = display_range_max_ - lower_bound + display_range_min_;
      }
    } else {
      std::ranges::reverse(color_lower_bounds_);
    }
  } else {
    const double step = (display_range_max_ - display_range_min_) / color_count;
    for (int i = 0; i <= color_count; i++) {
      color_lower_bounds_[i] = display_range_min_ + i * step;
    }
  }

  markColorsInvalid();
}

void HeatMapDataSource::assignMapColors()
{
  for (const auto& map_col : map_) {
    for (const auto& map_pt : map_col) {
      map_pt->color = getColor(map_pt->value);
    }
  }
  colors_correct_ = true;
}

double HeatMapDataSource::getRealRangeMinimumValue() const
{
  return color_lower_bounds_[0];
}

double HeatMapDataSource::getRealRangeMaximumValue() const
{
  return color_lower_bounds_[color_lower_bounds_.size() - 1];
}

std::vector<std::pair<int, double>> HeatMapDataSource::getLegendValues() const
{
  const int color_count = color_generator_.getColorCount();
  const int count = 6;
  std::vector<std::pair<int, double>> values;
  const double index_incr = static_cast<double>(color_count) / (count - 1);
  const double linear_start = getRealRangeMinimumValue();
  const double linear_step
      = (getRealRangeMaximumValue() - linear_start) / (count - 1);
  for (int i = 0; i < count; i++) {
    int idx = std::round(i * index_incr);
    idx = std::min(idx, color_count);
    double value = color_lower_bounds_[idx];
    if (!log_scale_) {
      value = linear_step * i + linear_start;
    }

    values.emplace_back(idx, value);
  }
  return values;
}

void HeatMapDataSource::onShow()
{
  if (!isPopulated()) {
    logger_->warn(utl::GUI,
                  66,
                  "Heat map \"{}\" has not been populated with data.",
                  getName());
  }
}

void HeatMapDataSource::onHide()
{
  if (destroyMapOnNotVisible()) {
    setIssueRedraw(false);
    destroyMap();
    setIssueRedraw(true);
  }
}

RealValueHeatMapDataSource::RealValueHeatMapDataSource(
    utl::Logger* logger,
    const std::string& unit_suffix,
    const std::string& name,
    const std::string& short_name,
    const std::string& settings_group)
    : HeatMapDataSource(logger, name, short_name, settings_group),
      unit_suffix_(unit_suffix),
      units_(unit_suffix_),
      min_(0.0),
      max_(0.0),
      scale_(1.0)
{
}

void RealValueHeatMapDataSource::correctMapScale(HeatMapDataSource::Map& map)
{
  determineMinMax(map);
  determineUnits();

  for (const auto& map_col : map) {
    for (const auto& map_pt : map_col) {
      map_pt->value = convertValueToPercent(map_pt->value);
    }
  }

  min_ = roundData(min_ * scale_);
  max_ = roundData(max_ * scale_);
  scale_ = 1.0;
}

double RealValueHeatMapDataSource::roundData(double value) const
{
  const double precision = 1000.0;
  return std::round(value * precision) / precision;
}

void RealValueHeatMapDataSource::determineMinMax(
    const HeatMapDataSource::Map& map)
{
  min_ = std::numeric_limits<double>::max();
  max_ = std::numeric_limits<double>::lowest();

  for (const auto& map_col : map) {
    for (const auto& map_pt : map_col) {
      min_ = std::min(min_, map_pt->value);
      max_ = std::max(max_, map_pt->value);
    }
  }
}

void RealValueHeatMapDataSource::determineUnits()
{
  const double range = getValueRange();
  if (range >= 1.0 || range == 0) {
    units_ = "";
    scale_ = 1.0;
  } else if (range > 1e-3) {
    units_ = "m";
    scale_ = 1e3;
  } else if (range > 1e-6) {
    units_ = "u";
    scale_ = 1e6;
  } else if (range > 1e-9) {
    units_ = "n";
    scale_ = 1e9;
  } else if (range > 1e-12) {
    units_ = "p";
    scale_ = 1e12;
  } else {
    units_ = "f";
    scale_ = 1e15;
  }

  units_ += unit_suffix_;
}

std::string RealValueHeatMapDataSource::formatValue(double value,
                                                    bool legend) const
{
  const int digits = legend ? 3 : 2;
  std::string text = formatFixed(convertPercentToValue(value), digits);
  if (legend) {
    text += getValueUnits();
  }
  return text;
}

std::string RealValueHeatMapDataSource::getValueUnits() const
{
  return units_;
}

double RealValueHeatMapDataSource::getValueRange() const
{
  double range = max_ - min_;
  if (range == 0.0) {
    range = 1.0;
  }
  return range;
}

double RealValueHeatMapDataSource::convertValueToPercent(double value) const
{
  const double range = getValueRange();
  const double offset = min_;

  return roundData(100.0 * (value - offset) / range);
}

double RealValueHeatMapDataSource::convertPercentToValue(double percent) const
{
  const double range = getValueRange();
  const double offset = min_;

  return roundData(percent * range / 100.0 + offset);
}

double RealValueHeatMapDataSource::getDisplayRangeIncrement() const
{
  return getValueRange() / 100.0;
}

GlobalRoutingDataSource::GlobalRoutingDataSource(
    utl::Logger* logger,
    const std::string& name,
    const std::string& short_name,
    const std::string& settings_group)
    : HeatMapDataSource(logger, name, short_name, settings_group)
{
}

std::pair<double, double> GlobalRoutingDataSource::getReportableXYGrid() const
{
  if (getBlock() == nullptr) {
    return {kDefaultGrid, kDefaultGrid};
  }

  auto* gcell_grid = getBlock()->getGCellGrid();
  if (gcell_grid == nullptr) {
    return {kDefaultGrid, kDefaultGrid};
  }

  auto grid_mode = [gcell_grid](int num_grids,
                                void (odb::dbGCellGrid::*get_grid)(
                                    int, int&, int&, int&)) -> int {
    std::map<int, int> grid_pitch_count;
    for (int i = 0; i < num_grids; i++) {
      int origin;
      int count;
      int step;
      (gcell_grid->*get_grid)(i, origin, count, step);
      grid_pitch_count[step] += count;
    }

    if (grid_pitch_count.empty()) {
      return kDefaultGrid;
    }

    auto mode = grid_pitch_count.begin();
    for (auto check_mode = grid_pitch_count.begin();
         check_mode != grid_pitch_count.end();
         check_mode++) {
      if (mode->second < check_mode->second) {
        mode = check_mode;
      }
    }
    return mode->first;
  };

  const double x_grid = grid_mode(gcell_grid->getNumGridPatternsX(),
                                  &odb::dbGCellGrid::getGridPatternX);
  const double y_grid = grid_mode(gcell_grid->getNumGridPatternsY(),
                                  &odb::dbGCellGrid::getGridPatternY);

  const double dbus = getBlock()->getDbUnitsPerMicron();
  return {x_grid / dbus, y_grid / dbus};
}

double GlobalRoutingDataSource::getGridXSize() const
{
  const auto& [x, y] = getReportableXYGrid();
  return x;
}

double GlobalRoutingDataSource::getGridYSize() const
{
  const auto& [x, y] = getReportableXYGrid();
  return y;
}

void GlobalRoutingDataSource::populateXYGrid()
{
  if (getBlock() == nullptr) {
    HeatMapDataSource::populateXYGrid();
    return;
  }

  auto* gcell_grid = getBlock()->getGCellGrid();
  if (gcell_grid == nullptr) {
    HeatMapDataSource::populateXYGrid();
    return;
  }

  std::vector<int> gcell_xgrid;
  std::vector<int> gcell_ygrid;
  gcell_grid->getGridX(gcell_xgrid);
  gcell_grid->getGridY(gcell_ygrid);

  const auto die_area = getBlock()->getDieArea();
  gcell_xgrid.push_back(die_area.xMax());
  gcell_ygrid.push_back(die_area.yMax());

  setXYMapGrid(gcell_xgrid, gcell_ygrid);
}

PowerDensityDataSource::PowerDensityDataSource(sta::dbSta* sta,
                                               utl::Logger* logger)
    : gui::RealValueHeatMapDataSource(logger,
                                      "W",
                                      "Power Density",
                                      "Power",
                                      "PowerDensity"),
      sta_(sta)
{
  setIssueRedraw(false);
  setLogScale(true);
  setIssueRedraw(true);

  addMultipleChoiceSetting(
      "Scene",
      "Scene:",
      [this]() {
        std::vector<std::string> scenes;
        for (auto* scene : sta_->scenes()) {
          scenes.emplace_back(scene->name());
        }
        return scenes;
      },
      [this]() -> std::string { return scene_; },
      [this](const std::string& value) { scene_ = value; });
  addBooleanSetting(
      "Internal",
      "Internal power:",
      [this]() { return include_internal_; },
      [this](bool value) { include_internal_ = value; });
  addBooleanSetting(
      "Leakage",
      "Leakage power:",
      [this]() { return include_leakage_; },
      [this](bool value) { include_leakage_ = value; });
  addBooleanSetting(
      "Switching",
      "Switching power:",
      [this]() { return include_switching_; },
      [this](bool value) { include_switching_ = value; });

  if (sta_ != nullptr) {
    for (auto* scene : sta_->scenes()) {
      if (scene != nullptr) {
        scene_ = scene->name();
        break;
      }
    }
  }
}

bool PowerDensityDataSource::populateMap()
{
  if (getBlock() == nullptr || sta_ == nullptr) {
    return false;
  }

  if (sta_->cmdNetwork() == nullptr) {
    return false;
  }

  auto* network = sta_->getDbNetwork();
  auto* scene = getScene();
  if (scene == nullptr) {
    return false;
  }

  // Collect selected instances if filter is enabled
  const std::set<odb::dbInst*> selected_insts = getSelectedInsts();
  const bool filter = !selected_insts.empty();

  const bool include_all
      = include_internal_ && include_leakage_ && include_switching_;
  for (auto* inst : getBlock()->getInsts()) {
    if (!inst->getPlacementStatus().isPlaced()) {
      continue;
    }
    if (filter && selected_insts.find(inst) == selected_insts.end()) {
      continue;
    }

    sta::PowerResult power = sta_->power(network->dbToSta(inst), scene);

    float pwr = 0.0;
    if (include_all) {
      pwr = power.total();
    } else {
      if (include_internal_) {
        pwr += power.internal();
      }
      if (include_leakage_) {
        pwr += power.switching();
      }
      if (include_switching_) {
        pwr += power.leakage();
      }
    }

    odb::Rect inst_box = inst->getBBox()->getBox();
    addToMap(inst_box, pwr);
  }

  return true;
}

void PowerDensityDataSource::combineMapData(bool,
                                            double& base,
                                            const double new_data,
                                            const double data_area,
                                            const double intersection_area,
                                            const double)
{
  base += (new_data / data_area) * intersection_area;
}

sta::Scene* PowerDensityDataSource::getScene() const
{
  if (scene_.empty()) {
    return nullptr;
  }
  for (auto* scene : sta_->scenes()) {
    if (scene->name() == scene_) {
      return scene;
    }
  }
  return nullptr;
}

HeatMapSourceRegistration::HeatMapSourceRegistration(std::string name,
                                                     std::string short_name,
                                                     std::string settings_group,
                                                     Factory factory)
    : name_(std::move(name)),
      short_name_(std::move(short_name)),
      settings_group_(std::move(settings_group)),
      factory_(std::move(factory))
{
}

std::shared_ptr<HeatMapDataSource> HeatMapSourceRegistration::createInstance()
    const
{
  std::lock_guard<std::mutex> lock(instances_mutex_);
  auto instance = factory_();
  instances_.erase(
      std::remove_if(instances_.begin(),
                     instances_.end(),
                     [](const std::weak_ptr<HeatMapDataSource>& candidate) {
                       return candidate.expired();
                     }),
      instances_.end());
  instances_.push_back(instance);
  return instance;
}

void HeatMapSourceRegistration::invalidateInstances() const
{
  std::lock_guard<std::mutex> lock(instances_mutex_);
  auto keep = instances_.begin();
  for (auto it = instances_.begin(); it != instances_.end(); ++it) {
    if (auto instance = it->lock()) {
      instance->update();
      *keep++ = *it;
    }
  }
  instances_.erase(keep, instances_.end());
}

HeatMapSourceHandle registerHeatMapSource(
    const std::string& name,
    const std::string& short_name,
    const std::string& settings_group,
    const HeatMapSourceRegistration::Factory& factory)
{
  std::lock_guard<std::mutex> lock(heatMapSourceMutex());
  for (const auto& source : heatMapSources()) {
    if (source->getShortName() == short_name) {
      return source;
    }
  }

  auto source = std::make_shared<HeatMapSourceRegistration>(
      name, short_name, settings_group, factory);
  heatMapSources().push_back(source);
  return source;
}

const std::vector<HeatMapSourceHandle>& getRegisteredHeatMapSources()
{
  return heatMapSources();
}

HeatMapSourceHandle findRegisteredHeatMapSource(const std::string& short_name)
{
  std::lock_guard<std::mutex> lock(heatMapSourceMutex());
  for (const auto& source : heatMapSources()) {
    if (source->getShortName() == short_name) {
      return source;
    }
  }
  return nullptr;
}

void registerBuiltinHeatMapSources(sta::dbSta* sta, utl::Logger* logger)
{
  registerHeatMapSource("Pin Density", "Pin", "PinDensity", [logger]() {
    return std::make_shared<PinDensityDataSource>(logger);
  });
  registerHeatMapSource(
      "Placement Density", "Placement", "PlacementDensity", [logger]() {
        return std::make_shared<PlacementDensityDataSource>(logger);
      });
  if (sta != nullptr) {
    registerHeatMapSource(
        "Power Density", "Power", "PowerDensity", [sta, logger]() {
          return std::make_shared<PowerDensityDataSource>(sta, logger);
        });
  }
}

}  // namespace gui
