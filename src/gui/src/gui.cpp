// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "gui/gui.h"

#include <QApplication>
#include <QColor>
#include <QImage>
#include <QPushButton>
#include <QString>
#include <QWidget>
#include <algorithm>
#include <any>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <map>
#include <memory>
#include <typeindex>
#include <typeinfo>
#include <utility>
#include <variant>

#include "gui/descriptor_registry.h"
#include "gui/heatMap.h"
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QRegularExpression>
#else
#include <QRegExp>
#endif
#include <cmath>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "boost/algorithm/string/predicate.hpp"
#include "chartsWidget.h"
#include "clockWidget.h"
#include "displayControls.h"
#include "drcWidget.h"
#include "gui_utils.h"
#include "heatMapGui.h"
#include "helpWidget.h"
#include "inspector.h"
#include "layoutViewer.h"
#include "mainWindow.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbShape.h"
#include "odb/geom.h"
#include "ord/OpenRoad.hh"
#include "qtDialogs.h"
#include "ruler.h"
#include "scriptWidget.h"
#include "timingWidget.h"
#include "utl/Logger.h"
#include "utl/decode.h"
#include "utl/exception.h"

extern int cmd_argc;
extern char** cmd_argv;

namespace gui {

static QApplication* application = nullptr;
static void message_handler(QtMsgType type,
                            const QMessageLogContext& context,
                            const QString& msg)
{
  auto* logger = ord::OpenRoad::openRoad()->getLogger();

  bool suppress = false;
#if NDEBUG
  // suppress messages when built as a release, but preserve them in debug
  // builds
  if (application != nullptr) {
    if (QApplication::platformName() == "offscreen"
        && msg.contains("This plugin does not support")) {
      suppress = true;
    }
  }

  // A Bazel-installed binary has no reachable ICU data directory (see
  // bazel/icu-patches), so QCollator legitimately cannot open a collator.
  // Qt already falls back to a plain, non-locale-aware string compare in
  // that case (qcollator_icu.cpp), so this is harmless -- just noisy.
  if (msg.contains("Could not create collator")) {
    suppress = true;
  }
#endif

  if (suppress) {
    return;
  }

  std::string print_msg;
  if (context.file != nullptr && context.function != nullptr) {
    print_msg = fmt::format("{}:{}:{}: {}",
                            context.file,
                            context.function,
                            context.line,
                            msg.toStdString());
  } else {
    print_msg = msg.toStdString();
  }
  switch (type) {
    case QtDebugMsg:
      debugPrint(logger, utl::GUI, "qt", 1, print_msg);
      break;
    case QtInfoMsg:
      logger->info(utl::GUI, 75, "{}", print_msg);
      break;
    case QtWarningMsg:
      logger->warn(utl::GUI, 76, "{}", print_msg);
      break;
    case QtCriticalMsg:
    case QtFatalMsg:
      logger->error(utl::GUI, 77, "{}", print_msg);
      break;
  }
}

// This provides the link for Gui::redraw to the widget
static gui::MainWindow* main_window = nullptr;

// Bridges Gui to the Qt main window.  Installed once the window is built and
// uninstalled before it is destroyed, so main_window is non-null and fully
// alive for every call below -- which is why none of them check it.
class QtGuiBackend : public GuiBackend
{
 public:
  bool hasWindow() const override { return true; }

  void redraw() override { main_window->redraw(); }

  void pause(int timeout_ms) override { main_window->pause(timeout_ms); }

  bool isPaused() const override
  {
    return main_window->getScriptWidget()->isPaused();
  }

  void status(const std::string& message) override
  {
    main_window->status(message);
  }

  void registerRenderer(Renderer* renderer) override
  {
    main_window->getControls()->registerRenderer(renderer);
  }

  void unregisterRenderer(Renderer* renderer) override
  {
    main_window->getControls()->unregisterRenderer(renderer);
  }

  void setSelected(const Selected& selection) override
  {
    main_window->setSelected(selection);
  }

  void addSelected(const Selected& selection) override
  {
    main_window->addSelected(selection);
  }

  void removeSelectedByType(const std::string& type) override
  {
    main_window->removeSelectedByType(type);
  }

  const SelectionSet& selection() override { return main_window->selection(); }

  const Selected& inspectorSelection() override
  {
    return main_window->getInspector()->getSelection();
  }

  bool anyObjectInSet(bool selection_set,
                      odb::dbObjectType obj_type) const override
  {
    return main_window->anyObjectInSet(selection_set, obj_type);
  }

  void addHighlighted(const SelectionSet& selection,
                      int highlight_group) override
  {
    main_window->addHighlighted(selection, highlight_group);
  }

  void clearHighlighted(int highlight_group) override
  {
    main_window->clearHighlighted(highlight_group);
  }

  void selectHighlightConnectedInsts(bool select_flag,
                                     int highlight_group) override
  {
    main_window->selectHighlightConnectedInsts(select_flag, highlight_group);
  }

  void selectHighlightConnectedNets(bool select_flag,
                                    bool output,
                                    bool input,
                                    int highlight_group) override
  {
    main_window->selectHighlightConnectedNets(
        select_flag, output, input, highlight_group);
  }

  void selectHighlightConnectedBufferTrees(bool select_flag,
                                           int highlight_group) override
  {
    main_window->selectHighlightConnectedBufferTrees(select_flag,
                                                     highlight_group);
  }

  int selectArea(const odb::Rect& area, bool append) override
  {
    return main_window->getLayoutViewer()->selectArea(area, append);
  }

  int selectNext() override
  {
    return main_window->getInspector()->selectNext();
  }

  int selectPrevious() override
  {
    return main_window->getInspector()->selectPrevious();
  }

  void selectionAnimation(int repeat) override
  {
    main_window->getLayoutViewer()->selectionAnimation(repeat);
  }

  void zoomTo(const odb::Rect& rect_dbu) override
  {
    main_window->zoomTo(rect_dbu);
  }

  void zoomTo(const odb::Point& focus, int diameter) override
  {
    main_window->zoomTo(focus, diameter);
  }

  void zoomIn() override { main_window->getLayoutViewer()->zoomIn(); }

  void zoomIn(const odb::Point& focus_dbu) override
  {
    main_window->getLayoutViewer()->zoomIn(focus_dbu);
  }

  void zoomOut() override { main_window->getLayoutViewer()->zoomOut(); }

  void zoomOut(const odb::Point& focus_dbu) override
  {
    main_window->getLayoutViewer()->zoomOut(focus_dbu);
  }

  void centerAt(const odb::Point& focus_dbu) override
  {
    main_window->getLayoutViewer()->centerAt(focus_dbu);
  }

  void setResolution(double pixels_per_dbu) override
  {
    main_window->getLayoutViewer()->setResolution(pixels_per_dbu);
  }

  void fit() override { main_window->fit(); }

  std::string addLabel(int x,
                       int y,
                       const std::string& text,
                       std::optional<Painter::Color> color,
                       std::optional<int> size,
                       std::optional<Painter::Anchor> anchor,
                       const std::optional<std::string>& name) override
  {
    return main_window->addLabel(x, y, text, color, size, anchor, name);
  }

  void deleteLabel(const std::string& name) override
  {
    main_window->deleteLabel(name);
  }

  void clearLabels() override { main_window->clearLabels(); }

  std::string addRuler(int x0,
                       int y0,
                       int x1,
                       int y1,
                       const std::string& label,
                       const std::string& name,
                       bool euclidian) override
  {
    return main_window->addRuler(x0, y0, x1, y1, label, name, euclidian);
  }

  void deleteRuler(const std::string& name) override
  {
    main_window->deleteRuler(name);
  }

  void clearRulers() override { main_window->clearRulers(); }

  bool checkDisplayControlVisible(const std::string& name) override
  {
    return main_window->getControls()->checkControlByPath(name, true);
  }

  bool checkDisplayControlSelectable(const std::string& name) override
  {
    return main_window->getControls()->checkControlByPath(name, false);
  }

  void setDisplayControlVisible(const std::string& name, bool value) override
  {
    main_window->getControls()->setControlByPath(
        name, true, value ? Qt::Checked : Qt::Unchecked);
  }

  void setDisplayControlSelectable(const std::string& name, bool value) override
  {
    main_window->getControls()->setControlByPath(
        name, false, value ? Qt::Checked : Qt::Unchecked);
  }

  void setDisplayControlColor(const std::string& name,
                              const Painter::Color& color) override
  {
    main_window->getControls()->setControlByPath(name, toQColor(color));
  }

  void saveDisplayControls() override { main_window->getControls()->save(); }

  void restoreDisplayControls() override
  {
    main_window->getControls()->restore();
  }

  void addFocusNet(odb::dbNet* net) override
  {
    main_window->getLayoutTabs()->addFocusNet(net);
  }

  void removeFocusNet(odb::dbNet* net) override
  {
    main_window->getLayoutTabs()->removeFocusNet(net);
  }

  void clearFocusNets() override
  {
    main_window->getLayoutTabs()->clearFocusNets();
  }

  void addRouteGuides(odb::dbNet* net) override
  {
    main_window->getLayoutTabs()->addRouteGuides(net);
  }

  void removeRouteGuides(odb::dbNet* net) override
  {
    main_window->getLayoutTabs()->removeRouteGuides(net);
  }

  void clearRouteGuides() override
  {
    main_window->getLayoutTabs()->clearRouteGuides();
  }

  void addNetTracks(odb::dbNet* net) override
  {
    main_window->getLayoutTabs()->addNetTracks(net);
  }

  void removeNetTracks(odb::dbNet* net) override
  {
    main_window->getLayoutTabs()->removeNetTracks(net);
  }

  void clearNetTracks() override
  {
    main_window->getLayoutTabs()->clearNetTracks();
  }

  void saveClockTreeImage(const std::string& clock_name,
                          const std::string& filename,
                          const std::string& scene,
                          std::optional<int> width_px,
                          std::optional<int> height_px) override
  {
    main_window->getClockViewer()->saveImage(
        clock_name, filename, scene, width_px, height_px);
  }

  void saveHistogramImage(const std::string& filename,
                          const std::string& mode,
                          std::optional<int> width_px,
                          std::optional<int> height_px) override
  {
    auto* charts = main_window->getChartsWidget();
    charts->saveImage(
        filename, charts->modeFromString(mode), width_px, height_px);
  }

  bool isOffscreen() const override
  {
    // Set when the gui was started non-interactively.
    return main_window->testAttribute(Qt::WA_DontShowOnScreen);
  }

  RenderedImage renderImage(
      const odb::Rect& region,
      int width_px,
      double dbu_per_pixel,
      std::optional<std::pair<int, int>> scale_to) override
  {
    QImage img = main_window->getLayoutViewer()->createImage(
        region, width_px, dbu_per_pixel);
    if (scale_to.has_value()) {
      img = img.scaled(scale_to->first, scale_to->second, Qt::KeepAspectRatio);
    }

    // Format_RGBA8888 is byte-ordered, so its memory layout is already the
    // R,G,B,A this hands back and the rows can be copied whole.  Going
    // through pixel() instead costs a bounds check, a format check and a
    // coordinate mapping for each of the hundreds of thousands of pixels in
    // a frame.  Copying per row rather than in one block avoids assuming
    // bytesPerLine() has no padding.
    const QImage rgba = img.convertToFormat(QImage::Format_RGBA8888);

    RenderedImage out;
    out.width = rgba.width();
    out.height = rgba.height();
    const size_t row_bytes = static_cast<size_t>(out.width) * 4;
    out.rgba.resize(row_bytes * out.height);
    for (int y = 0; y < out.height; y++) {
      std::copy_n(rgba.constScanLine(y), row_bytes, &out.rgba[row_bytes * y]);
    }
    return out;
  }
};

static QtGuiBackend qt_backend;

void Gui::setChartFactory(ChartFactory factory)
{
  chart_factory_ = std::move(factory);
}

/**
 * @brief Checks if a Qt wildcard pattern is a simple literal string.
 *
 * This function determines if a string intended for use with
 * QRegExp::WildcardUnix contains any active (i.e., unescaped) wildcard
 * characters ('*', '?', '[').
 *
 * @param pattern The wildcard pattern string to check.
 * @return True if the pattern has no active wildcards; false otherwise.
 */
static bool isSimpleStringPattern(const std::string& pattern)
{
  bool previous_was_escape = false;
  for (const char ch : pattern) {
    if (previous_was_escape) {
      // The previous character was '\', so this character is just a literal.
      previous_was_escape = false;
      continue;
    }

    if (ch == '\\') {
      // This is an escape character for the next character in the loop.
      previous_was_escape = true;
    } else if (ch == '*' || ch == '?' || ch == '[') {
      // Found an unescaped wildcard, so it's not a simple string.
      return false;
    }
  }
  // If the loop completes, no unescaped wildcards were found.
  return true;
}

int Gui::select(const std::string& type,
                const std::string& name_filter,
                const std::string& attribute,
                const std::any& value,
                bool filter_case_sensitive,
                int highlight_group)
{
  if (!hasUI()) {
    return 0;
  }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  // Define case sensitivity options for QRegularExpression
  const QRegularExpression::PatternOptions options
      = filter_case_sensitive ? QRegularExpression::NoPatternOption
                              : QRegularExpression::CaseInsensitiveOption;

  // Convert the wildcard string to a regex pattern and create the
  // object
  const QRegularExpression reg_filter(
      QRegularExpression::wildcardToRegularExpression(
          QString::fromStdString(name_filter)),
      options);
#else
  const QRegExp reg_filter(
      QString::fromStdString(name_filter),
      filter_case_sensitive ? Qt::CaseSensitive : Qt::CaseInsensitive,
      QRegExp::WildcardUnix);
#endif
  const bool is_simple = isSimpleStringPattern(name_filter);
  bool found = false;
  int result = 0;
  auto* registry = DescriptorRegistry::instance();
  registry->forEachDescriptor([&](const Descriptor* descriptor) {
    if (found || descriptor->getTypeName() != type) {
      return;
    }
    found = true;
    SelectionSet selected_set;
    descriptor->visitAllObjects([&](const Selected& sel) {
      if (!name_filter.empty()) {
        const std::string sel_name = sel.getName();
        if (is_simple) {
          if (sel_name != name_filter) {
            return;
          }
        } else {
          if (
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
              !reg_filter.match(QString::fromStdString(sel_name)).hasMatch()
#else
              !reg_filter.exactMatch(QString::fromStdString(sel_name))
#endif
          ) {
            return;
          }
        }
      }

      if (!attribute.empty()) {
        bool is_valid_attribute = false;
        Descriptor::Properties properties
            = descriptor->getProperties(sel.getObject());
        if (!filterSelectionProperties(
                properties, attribute, value, is_valid_attribute)) {
          return;  // doesn't match the attribute filter
        }

        if (!is_valid_attribute) {
          logger_->error(
              utl::GUI, 59, "Entered attribute {} is not valid.", attribute);
        }
      }
      selected_set.insert(sel);
    });

    main_window->addSelected(selected_set, true);
    if (highlight_group != -1) {
      main_window->addHighlighted(selected_set, highlight_group);
    }

    result = selected_set.size();
  });

  if (!found) {
    logger_->error(utl::GUI, 35, "Unable to find descriptor for: {}", type);
  }
  return result;
}

bool Gui::filterSelectionProperties(const Descriptor::Properties& properties,
                                    const std::string& attribute,
                                    const std::any& value,
                                    bool& is_valid_attribute)
{
  for (const Descriptor::Property& property : properties) {
    if (attribute == property.name) {
      is_valid_attribute = true;
      if (auto props_selected_set
          = std::any_cast<SelectionSet>(&property.value)) {
        if (Descriptor::Property::toString(value) == "CONNECTED"
            && !props_selected_set->empty()) {
          return true;
        }
        for (const auto& selected : *props_selected_set) {
          if (Descriptor::Property::toString(value) == selected.getName()) {
            return true;
          }
        }
      } else if (auto props_list
                 = std::any_cast<Descriptor::PropertyList>(&property.value)) {
        for (const auto& prop : *props_list) {
          if (Descriptor::Property::toString(prop.first)
                  == Descriptor::Property::toString(value)
              || Descriptor::Property::toString(prop.second)
                     == Descriptor::Property::toString(value)) {
            return true;
          }
        }
      } else if (Descriptor::Property::toString(value)
                 == Descriptor::Property::toString(property.value)) {
        return true;
      }
    }
  }

  return false;
}

std::string Gui::addToolbarButton(const std::string& name,
                                  const std::string& text,
                                  const std::string& script,
                                  bool echo)
{
  if (!hasUI()) {
    return "";
  }
  return main_window->addToolbarButton(
      name, QString::fromStdString(text), QString::fromStdString(script), echo);
}

void Gui::removeToolbarButton(const std::string& name)
{
  if (!hasUI()) {
    return;
  }
  main_window->removeToolbarButton(name);
}

std::string Gui::addMenuItem(const std::string& name,
                             const std::string& path,
                             const std::string& text,
                             const std::string& script,
                             const std::string& shortcut,
                             bool echo)
{
  if (!hasUI()) {
    return "";
  }
  return main_window->addMenuItem(name,
                                  QString::fromStdString(path),
                                  QString::fromStdString(text),
                                  QString::fromStdString(script),
                                  QString::fromStdString(shortcut),
                                  echo);
}

void Gui::removeMenuItem(const std::string& name)
{
  if (!hasUI()) {
    return;
  }
  main_window->removeMenuItem(name);
}

std::string Gui::requestUserInput(const std::string& title,
                                  const std::string& question)
{
  if (!hasUI()) {
    return "";
  }
  return main_window->requestUserInput(QString::fromStdString(title),
                                       QString::fromStdString(question));
}

void Gui::selectMarkers(odb::dbMarkerCategory* markers)
{
  if (!hasUI()) {
    return;
  }
  main_window->getDRCViewer()->selectCategory(markers);
}

void Gui::saveImage(const std::string& filename,
                    const odb::Rect& region,
                    int width_px,
                    double dbu_per_pixel,
                    const std::map<std::string, bool>& display_settings)
{
  if (db_ == nullptr) {
    logger_->error(utl::GUI, 15, "No design loaded.");
  }
  odb::Rect save_region = region;
  const bool use_die_area = region.dx() == 0 || region.dy() == 0;
  const bool is_offscreen
      = main_window == nullptr
        || main_window->testAttribute(
            Qt::WA_DontShowOnScreen); /* if not interactive this will be set */
  if (is_offscreen
      && use_die_area) {  // if gui is active and interactive the visible are of
                          // the layout viewer will be used.
    auto* chip = db_->getChip();
    if (chip == nullptr) {
      logger_->error(utl::GUI, 64, "No design loaded.");
    }
    save_region = chip->getBBox();
    auto* block = chip->getBlock();

    if (block != nullptr) {
      save_region = block->getBBox()->getBox();
    }

    // get die area since screen area is not reliable
    const double bloat_by = 0.05;  // 5%
    const int bloat = std::min(save_region.dx(), save_region.dy()) * bloat_by;

    save_region.bloat(bloat, save_region);
  }

  if (!hasUI()) {
    const double dbu_per_micron = db_->getDbuPerMicron();

    std::string save_cmds;

    // build display control commands
    save_cmds = "set ::gui::display_settings [gui::DisplayControlMap]\n";
    for (const auto& [control, value] : display_settings) {
      // first save current setting
      save_cmds += fmt::format(
                       "$::gui::display_settings set \"{}\" {}", control, value)
                   + "\n";
    }
    // save command
    save_cmds += "gui::save_image ";
    save_cmds += "\"" + filename + "\" ";
    save_cmds += std::to_string(save_region.xMin() / dbu_per_micron) + " ";
    save_cmds += std::to_string(save_region.yMin() / dbu_per_micron) + " ";
    save_cmds += std::to_string(save_region.xMax() / dbu_per_micron) + " ";
    save_cmds += std::to_string(save_region.yMax() / dbu_per_micron) + " ";
    save_cmds += std::to_string(width_px) + " ";
    save_cmds += std::to_string(dbu_per_pixel) + " ";
    save_cmds += "$::gui::display_settings\n";
    // delete display settings map
    save_cmds += "rename $::gui::display_settings \"\"\n";
    save_cmds += "unset ::gui::display_settings\n";
    // end with hide to return
    save_cmds += "gui::hide";
    showGui(save_cmds, false);
  } else {
    // save current display settings and apply new
    main_window->getControls()->save();
    for (const auto& [control, value] : display_settings) {
      setDisplayControlsVisible(control, value);
    }

    main_window->getLayoutViewer()->saveImage(
        filename.c_str(), save_region, width_px, dbu_per_pixel);
    // restore settings
    main_window->getControls()->restore();
  }
}

void Gui::showWorstTimingPath(bool setup)
{
  if (!hasUI()) {
    return;
  }
  main_window->getTimingWidget()->showWorstTimingPath(setup);
}

void Gui::clearTimingPath()
{
  if (!hasUI()) {
    return;
  }
  main_window->getTimingWidget()->clearSelection();
}

void Gui::selectClockviewerClock(const std::string& clock_name,
                                 std::optional<int> depth)
{
  if (!hasUI()) {
    return;
  }
  main_window->getClockViewer()->selectClock(clock_name, depth);
}

static QWidget* findWidget(const std::string& name)
{
  if (name == "main_window" || name == "OpenROAD") {
    return main_window;
  }

  if (main_window == nullptr) {
    return nullptr;
  }

  const QString find_name = QString::fromStdString(name);
  for (const auto& widget : main_window->findChildren<QDockWidget*>()) {
    if (widget->objectName() == find_name
        || widget->windowTitle() == find_name) {
      return widget;
    }
  }
  return nullptr;
}

void Gui::showWidget(const std::string& name, bool show)
{
  auto* widget = findWidget(name);
  if (widget == nullptr) {
    return;
  }

  if (show) {
    widget->show();
    widget->raise();
  } else {
    widget->hide();
  }
}

void Gui::triggerAction(const std::string& name)
{
  const size_t dot_idx = name.find_last_of('.');
  auto* widget = findWidget(name.substr(0, dot_idx));
  if (widget == nullptr) {
    return;
  }

  const QString find_name = QString::fromStdString(name.substr(dot_idx + 1));

  // Find QAction
  for (QAction* action : widget->findChildren<QAction*>()) {
    logger_->report("{} {}",
                    action->objectName().toStdString(),
                    action->text().toStdString());
    if (action->objectName() == find_name || action->text() == find_name) {
      action->trigger();
      return;
    }
  }

  // Find QPushButton
  for (QPushButton* button : widget->findChildren<QPushButton*>()) {
    if (button->objectName() == find_name || button->text() == find_name) {
      button->click();
      return;
    }
  }
}

void Gui::registerHeatMap(HeatMapDataSource* heatmap)
{
  if (heat_maps_.contains(heatmap)) {
    return;
  }
  heat_maps_.insert(heatmap);
  auto renderer = makeHeatMapRenderer(*heatmap);
  heatmap->setRedrawCallback(
      [renderer_ptr = renderer.get()]() { renderer_ptr->redraw(); });
  heatmap->setSetupCallback([heatmap]() { showHeatMapSetupDialog(heatmap); });
  heatmap->setUnregisterCallback(
      [this](HeatMapDataSource* source) { unregisterHeatMap(source); });
  registerRenderer(renderer.get());
  heat_map_renderers_[heatmap] = std::move(renderer);
  if (main_window != nullptr) {
    main_window->registerHeatMap(heatmap);
  }
}

void Gui::unregisterHeatMap(HeatMapDataSource* heatmap)
{
  if (!heat_maps_.contains(heatmap)) {
    return;
  }

  heatmap->setRedrawCallback({});
  heatmap->setSetupCallback({});
  heatmap->setUnregisterCallback({});
  auto renderer_itr = heat_map_renderers_.find(heatmap);
  if (renderer_itr != heat_map_renderers_.end()) {
    unregisterRenderer(renderer_itr->second.get());
    heat_map_renderers_.erase(renderer_itr);
  }
  if (main_window != nullptr) {
    main_window->unregisterHeatMap(heatmap);
  }
  heat_maps_.erase(heatmap);
}

void Gui::syncHeatMapChips()
{
  if (hasUI() || db_ == nullptr) {
    return;
  }

  // Console and headless sessions do not receive MainWindow::setBlock().
  auto* chip = db_->getChip();
  for (auto* heat_map : heat_maps_) {
    if (heat_map->getChip() != chip) {
      heat_map->setChip(chip);
      heat_map->destroyMap();
    }
  }
}

const std::set<HeatMapDataSource*>& Gui::getHeatMaps()
{
  syncHeatMapChips();
  return heat_maps_;
}

HeatMapDataSource* Gui::getHeatMap(const std::string& name)
{
  syncHeatMapChips();

  HeatMapDataSource* source = nullptr;

  for (auto* heat_map : heat_maps_) {
    if (heat_map->getShortName() == name) {
      source = heat_map;
      break;
    }
  }

  if (source == nullptr) {
    QStringList options;
    for (auto* heat_map : heat_maps_) {
      options.append(QString::fromStdString(heat_map->getShortName()));
    }
    logger_->error(utl::GUI,
                   28,
                   "{} is not a known map. Valid options are: {}",
                   name,
                   options.join(", ").toStdString());
  }

  return source;
}

void Gui::setHeatMapSetting(const std::string& name,
                            const std::string& option,
                            const Renderer::Setting& value)
{
  HeatMapDataSource* source = getHeatMap(name);

  const std::string rebuild_map_option = "rebuild";
  if (option == rebuild_map_option) {
    source->destroyMap();
    source->ensureMap();
  } else {
    auto settings = source->getSettings();

    if (!settings.contains(option)) {
      QStringList options;
      options.append(QString::fromStdString(rebuild_map_option));
      for (const auto& [key, kv] : settings) {
        options.append(QString::fromStdString(key));
      }
      logger_->error(utl::GUI,
                     29,
                     "{} is not a valid option. Valid options are: {}",
                     option,
                     options.join(", ").toStdString());
    }

    auto& current_value = settings[option];
    if (std::holds_alternative<bool>(current_value)) {
      // is bool
      if (auto* s = std::get_if<bool>(&value)) {
        settings[option] = *s;
      }
      if (auto* s = std::get_if<int>(&value)) {
        settings[option] = *s != 0;
      }
      if (auto* s = std::get_if<double>(&value)) {
        settings[option] = *s != 0.0;
      } else {
        logger_->error(utl::GUI, 60, "{} must be a boolean", option);
      }
    } else if (std::holds_alternative<int>(current_value)) {
      // is int
      if (auto* s = std::get_if<int>(&value)) {
        settings[option] = *s;
      } else if (auto* s = std::get_if<double>(&value)) {
        settings[option] = static_cast<int>(*s);
      } else {
        logger_->error(utl::GUI, 61, "{} must be an integer or double", option);
      }
    } else if (std::holds_alternative<double>(current_value)) {
      // is double
      if (auto* s = std::get_if<int>(&value)) {
        settings[option] = static_cast<double>(*s);
      } else if (auto* s = std::get_if<double>(&value)) {
        settings[option] = *s;
      } else {
        logger_->error(utl::GUI, 62, "{} must be an integer or double", option);
      }
    } else {
      // is string
      if (auto* s = std::get_if<std::string>(&value)) {
        settings[option] = *s;
      } else {
        logger_->error(utl::GUI, 63, "{} must be a string", option);
      }
    }
    source->setSettings(settings);
  }

  source->redraw();
}

Renderer::Setting Gui::getHeatMapSetting(const std::string& name,
                                         const std::string& option)
{
  HeatMapDataSource* source = getHeatMap(name);

  const std::string map_has_option = "has_data";
  if (option == map_has_option) {
    return source->hasData();
  }

  auto settings = source->getSettings();

  if (!settings.contains(option)) {
    QStringList options;
    for (const auto& [key, kv] : settings) {
      options.append(QString::fromStdString(key));
    }
    logger_->error(utl::GUI,
                   95,
                   "{} is not a valid option. Valid options are: {}",
                   option,
                   options.join(", ").toStdString());
  }

  return settings[option];
}

void Gui::dumpHeatMap(const std::string& name, const std::string& file)
{
  HeatMapDataSource* source = getHeatMap(name);
  source->dumpToFile(file);
}

void Gui::setMainWindowTitle(const std::string& title)
{
  main_window_title_ = title;
  if (main_window) {
    main_window->setTitle(title);
  }
}

std::string Gui::getMainWindowTitle()
{
  return main_window_title_;
}

void Gui::timingCone(Term term, bool fanin, bool fanout)
{
  if (!hasUI()) {
    return;
  }
  main_window->timingCone(term, fanin, fanout);
}

void Gui::timingPathsThrough(const std::set<Term>& terms)
{
  if (!hasUI()) {
    return;
  }
  main_window->timingPathsThrough(terms);
}

Chart* Gui::addChart(const std::string& name,
                     const std::string& x_label,
                     const std::vector<std::string>& y_labels)
{
  if (main_window != nullptr) {
    return main_window->getChartsWidget()->addChart(name, x_label, y_labels);
  }
  if (chart_factory_) {
    return chart_factory_(name, x_label, y_labels);
  }
  return nullptr;
}

void Gui::setLogger(utl::Logger* logger)
{
  if (logger == nullptr) {
    return;
  }

  logger_ = logger;
  qInstallMessageHandler(message_handler);

  if (hasUI()) {
    // gui already requested, so go ahead and set the logger
    main_window->setLogger(logger);
  }
}

void Gui::hideGui()
{
  if (!hasUI()) {
    return;
  }
  // ensure continue after close is true, since we want to return to tcl
  setContinueAfterClose();
  main_window->exit();
}

void Gui::showGui(const std::string& cmds, bool interactive, bool load_settings)
{
  if (enabled()) {
    logger_->warn(utl::GUI, 8, "GUI already active.");
    return;
  }

  // OR already running, so GUI should not set anything up
  // passing in cmd_argc and cmd_argv to meet Qt application requirement for
  // arguments nullptr for tcl interp to indicate nothing to setup and commands
  // and interactive setting
  startGui(cmd_argc, cmd_argv, nullptr, cmds, interactive, load_settings);
}

void Gui::minimize()
{
  if (!hasUI()) {
    return;
  }
  main_window->showMinimized();
}

void Gui::unminimize()
{
  if (!hasUI()) {
    return;
  }
  main_window->showNormal();
}

void Gui::init(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger)
{
  db_ = db;
  setLogger(logger);

  // Lets the descriptors offer the actions that need a modal dialog.  Only
  // this file is Qt-only, so a build without Qt leaves the hook null and
  // those actions are not offered.
  static QtDialogs dialogs;
  setDialogs(&dialogs);

  auto* registry = DescriptorRegistry::instance();
  registry->setLogger(logger);
  registry->initDescriptors(db, sta);

  registerBuiltinHeatMapSources(sta, logger);
  for (const auto& source : getRegisteredHeatMapSources()) {
    const bool already_registered = std::ranges::any_of(
        heat_maps_, [&source](HeatMapDataSource* heatmap) {
          return heatmap->getShortName() == source->getShortName();
        });
    if (already_registered) {
      continue;
    }
    auto instance = source->createInstance();
    instance->setChip(db->getChip());
    registerHeatMap(instance.get());
    owned_heat_maps_.push_back(std::move(instance));
  }
}

void Gui::selectHelp(const std::string& item)
{
  if (!hasUI()) {
    return;
  }

  main_window->getHelpViewer()->selectHelp(item);
}

void Gui::selectChart(const std::string& name)
{
  if (!hasUI()) {
    return;
  }

  const ChartsWidget::Mode mode
      = main_window->getChartsWidget()->modeFromString(name);
  main_window->getChartsWidget()->setMode(mode);
}

void Gui::updateTimingReport()
{
  if (!hasUI()) {
    return;
  }
  main_window->getTimingWidget()->populatePaths();
}

class SafeApplication : public QApplication
{
 public:
  using QApplication::QApplication;

  bool notify(QObject* receiver, QEvent* event) override
  {
    try {
      return QApplication::notify(receiver, event);
    } catch (std::exception& ex) {
      // Ignored here as the message will be logged in the GUI
      qDebug() << "Caught exception:" << ex.what();

      // Returning true indicates the event has been handled. In this case,
      // we've "handled" it by catching the exception, so we prevent
      // further processing that might rely on a corrupt state.
      return true;
    }

    return false;
  }
};

//////////////////////////////////////////////////

// This is the main entry point to start the GUI.  It only
// returns when the GUI is done.
int startGui(int& argc,
             char* argv[],
             Tcl_Interp* interp,
             const std::string& script,
             bool interactive,
             bool load_settings,
             bool minimize)
{
#ifdef STATIC_QPA_PLUGIN_XCB
  const char* qt_qpa_platform_env = getenv("QT_QPA_PLATFORM");
  std::string qpa_platform
      = qt_qpa_platform_env == nullptr ? "" : qt_qpa_platform_env;
  if (qpa_platform != "") {
    if (qpa_platform.find("xcb") == std::string::npos
        && qpa_platform.find("offscreen") == std::string::npos) {
      // OpenROAD logger is not available yet, using cout.
      std::cout << "Your system has set QT_QPA_PLATFORM='" << qpa_platform
                << "', openroad only supports 'offscreen' and 'xcb', please "
                   "include one of these plugins in your platform env\n";
    }
  }
#endif
  auto gui = gui::Gui::get();
  // ensure continue after close is false
  gui->clearContinueAfterClose();

  SafeApplication app(argc, argv);
  application = &app;

  // Default to 12 point for easier reading
  QFont font = QApplication::font();
  font.setPointSize(12);
  QApplication::setFont(font);

  auto* open_road = ord::OpenRoad::openRoad();

  // create new MainWindow
  main_window = new gui::MainWindow(load_settings);
  gui->setBackend(&qt_backend);
  if (minimize) {
    main_window->showMinimized();
  }
  main_window->setTitle(gui->getMainWindowTitle());

  open_road->getDb()->addObserver(main_window);
  if (!interactive) {
    gui->setContinueAfterClose();
    main_window->setAttribute(Qt::WA_DontShowOnScreen);
  }
  main_window->show();

  gui->setLogger(open_road->getLogger());

  main_window->setDatabase(open_road->getDb());

  bool init_openroad = interp != nullptr;
  if (!init_openroad) {
    interp = open_road->tclInterp();
  }

  // pass in tcl interp to script widget and ensure OpenRoad gets initialized
  main_window->getScriptWidget()->setupTcl(
      interp, interactive, init_openroad, [&]() {
        // init remainder of GUI, to be called immediately after OpenRoad is
        // guaranteed to be initialized.
        main_window->init(open_road->getSta(), open_road->getDocsPath());
        // announce design created to ensure GUI gets setup
        main_window->postReadDb(main_window->getDb());
      });

  // Exit the app if someone chooses exit from the menu in the window
  QObject::connect(main_window, &MainWindow::exit, &app, &QApplication::quit);
  // Track the exit in case it originated during a script
  bool exit_requested = false;
  int exit_code = EXIT_SUCCESS;
  QObject::connect(
      main_window, &MainWindow::exit, [&]() { exit_requested = true; });

  // Hide the Gui if someone chooses hide from the menu in the window
  QObject::connect(main_window, &MainWindow::hide, [gui]() { gui->hideGui(); });

  // Save the window's status into the settings when quitting.
  QObject::connect(
      &app, &QApplication::aboutToQuit, main_window, &MainWindow::saveSettings);

  // execute commands to restore state of gui
  std::string restore_commands;
  for (const auto& cmd : gui->getRestoreStateCommands()) {
    restore_commands += cmd + "\n";
  }
  if (!restore_commands.empty()) {
    // Temporarily connect to script widget to get ending tcl state
    bool tcl_ok = true;
    auto tcl_return_code_connect
        = QObject::connect(main_window->getScriptWidget(),
                           &ScriptWidget::commandExecuted,
                           [&tcl_ok](bool is_ok) { tcl_ok = is_ok; });

    main_window->getScriptWidget()->executeSilentCommand(
        QString::fromStdString(restore_commands));

    // disconnect tcl return lister
    QObject::disconnect(tcl_return_code_connect);

    if (!exit_requested && !tcl_ok) {
      auto& cmds = gui->getRestoreStateCommands();
      if (cmds[cmds.size() - 1]
          == "exit") {  // exit, will be the last command if it is present
        // if there was a failure and exit was requested, exit with failure
        // this will mirror the behavior of tclAppInit
        gui->clearContinueAfterClose();
        exit_code = EXIT_FAILURE;
        exit_requested = true;
      }
    }
  }

  // temporary storage for any exceptions thrown by scripts
  utl::ThreadException exception;
  // Execute script
  if (!script.empty() && !exit_requested) {
    try {
      main_window->getScriptWidget()->executeCommand(
          QString::fromStdString(script));
    } catch (const std::runtime_error& /* e */) {
      exception.capture();
    }
  }

  bool do_exec = interactive && !exception.hasException() && !exit_requested;
  // check if hide was called by script
  if (gui->isContinueAfterClose()) {
    do_exec = false;
  }

  if (do_exec) {
    exit_code = QApplication::exec();
  }

  // cleanup
  open_road->getDb()->removeObserver(main_window);

  if (!exception.hasException()) {
    // don't save anything if exception occured
    gui->clearRestoreStateCommands();
    // save restore state commands
    for (const auto& cmd : main_window->getRestoreTclCommands()) {
      gui->addRestoreStateCommand(cmd);
    }
  }

  main_window->exit();

  // Uninstall before destroying the window, not after.  ~MainWindow destroys
  // its children in construction order, so DisplayControls (the first one)
  // is already gone when DRCWidget and the clock viewer destroy the
  // Renderers they own.  Each ~Renderer calls Gui::unregisterRenderer, and
  // with the backend still installed that would reach
  // main_window->getControls() on a freed DisplayControls.
  Gui::get()->setBackend(nullptr);

  // delete main window and set to nullptr
  delete main_window;
  main_window = nullptr;
  application = nullptr;

  Gui::resetDbuConversions();

  // rethow exception, if one happened after cleanup of main_window
  exception.rethrow();

  debugPrint(open_road->getLogger(),
             utl::GUI,
             "init",
             1,
             "Exit state: interactive ({}), isContinueAfterClose ({}), "
             "exit_requested ({}), exit_code ({})",
             interactive,
             gui->isContinueAfterClose(),
             exit_requested,
             exit_code);

  const bool do_exit = !gui->isContinueAfterClose() && exit_requested;
  if (interactive && do_exit) {
    // if exiting, go ahead and exit with gui return code.
    exit(exit_code);
  }

  return exit_code;
}

// Tcl files encoded into strings.
extern const char* gui_tcl_inits[];

extern "C" {
extern int Gui_Init(Tcl_Interp* interp);
}

void initGui(Tcl_Interp* interp,
             odb::dbDatabase* db,
             sta::dbSta* sta,
             utl::Logger* logger)
{
  // Define swig TCL commands.
  Gui_Init(interp);
  utl::evalTclInit(interp, gui::gui_tcl_inits);

  // ensure gui is made
  auto* gui = gui::Gui::get();
  gui->init(db, sta, logger);
}

}  // namespace gui
