// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// This file is only used when we can't find Qt5 and are thus
// disabling the GUI.  It is not included when Qt5 is found.

#include <any>
#include <cstdio>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

#include "gui/descriptor_registry.h"
#include "gui/gui.h"
#include "gui/heatMap.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "tcl.h"

// empty gif writer class
struct GifWriter
{
};

namespace gui {

Gui::Gui() : continue_after_close_(false), logger_(nullptr), db_(nullptr)
{
}

void HeatMapDataSource::registerHeatMap()
{
  // gpl / other modules call this to expose their heatmap to the GUI.
  // In headless mode the web viewer enumerates heatmaps via
  // gui::getRegisteredHeatMapSources() (factory-backed sources) so this
  // one-off pathway does nothing here for now.  Left intentionally as
  // a no-op until heatmap plumbing for ad-hoc sources lands.
}

void gui::Gui::setChartFactory(ChartFactory factory)
{
  chart_factory_ = std::move(factory);
}

void Gui::triggerAction(const std::string& /* action */)
{
}

// using namespace odb;
int startGui(int& argc,
             char* argv[],
             Tcl_Interp* interp,
             const std::string& script,
             bool interactive,
             bool load_settings,
             bool minimize)
{
  printf(
      "[ERROR] This code was compiled with the GUI disabled.  Please recompile "
      "with Qt5 if you want the GUI.\n");

  return 1;  // return unix err
}

void initGui(Tcl_Interp* interp,
             odb::dbDatabase* db,
             sta::dbSta* sta,
             utl::Logger* logger)
{
  // Initialize the descriptor registry so that descriptors are available
  // for the web viewer and other non-GUI consumers.
  auto* registry = DescriptorRegistry::instance();
  registry->setLogger(logger);
  registry->initDescriptors(db, sta);
  registerBuiltinHeatMapSources(sta, logger);

  // Tcl requires this to be a writable string
  std::string cmd_save_image(
      "proc save_image { args } {"
      "  utl::error GUI 4 \"Command save_image is not available as OpenROAD "
      "was not compiled with QT support.\""
      "}");
  Tcl_Eval(interp, cmd_save_image.c_str());
  std::string cmd_supported(
      "namespace eval gui {"
      "  proc supported {} {"
      "    return 0"
      "  }"
      "}");
  Tcl_Eval(interp, cmd_supported.c_str());
  std::string enabled_supported(
      "namespace eval gui {"
      "  proc enabled {} {"
      "    return 0"
      "  }"
      "}");
  Tcl_Eval(interp, enabled_supported.c_str());
  // Counterpart of gui.i's has_ui: commands shared with a non-Qt viewer
  // dispatch on it, so it has to answer in a build with no Qt at all.
  std::string cmd_has_ui(
      "namespace eval gui {"
      "  proc has_ui {} {"
      "    return 0"
      "  }"
      "}");
  Tcl_Eval(interp, cmd_has_ui.c_str());
}

int Gui::gifStart(const std::string& filename)
{
  return 0;
}

void Gui::gifEnd(std::optional<int> key)
{
}

void Gui::gifAddFrame(std::optional<int> key,
                      const odb::Rect& region,
                      int width_px,
                      double dbu_per_pixel,
                      std::optional<int> delay)
{
}

Chart* Gui::addChart(const std::string& name,
                     const std::string& x_label,
                     const std::vector<std::string>& y_labels)
{
  if (chart_factory_) {
    return chart_factory_(name, x_label, y_labels);
  }
  return nullptr;
}

void Gui::saveImage(const std::string& filename,
                    const odb::Rect& region,
                    int width_px,
                    double dbu_per_pixel,
                    const std::map<std::string, bool>& display_settings)
{
}

int Gui::select(const std::string& type,
                const std::string& name_filter,
                const std::string& attribute,
                const std::any& value,
                bool filter_case_sensitive,
                int highlight_group)
{
  return 0;
}

// The display-control state belongs to whatever front-end is installed, which
// for a no-Qt binary is the headless viewer (e.g. the web viewer).  Without a
// viewer everything is visible so headless renderers draw by default.
void Gui::addFocusNet(odb::dbNet* net)
{
}

void Gui::removeFocusNet(odb::dbNet* net)
{
}

void Gui::addRouteGuides(odb::dbNet* net)
{
}

void Gui::removeRouteGuides(odb::dbNet* net)
{
}

void Gui::addNetTracks(odb::dbNet* net)
{
}

void Gui::removeNetTracks(odb::dbNet* net)
{
}

void Gui::timingCone(Term term, bool fanin, bool fanout)
{
}

void Gui::timingPathsThrough(const std::set<Term>& terms)
{
}

}  // namespace gui
