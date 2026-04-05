// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "layoutTabs.h"

#include <QColor>
#include <QWidget>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "colorGenerator.h"
#include "gui/gui.h"
#include "layoutViewer.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace gui {

LayoutTabs::LayoutTabs(Options* options,
                       ScriptWidget* output_widget,
                       const SelectionSet& selected,
                       const HighlightSet& highlighted,
                       const std::vector<std::unique_ptr<Ruler>>& rulers,
                       const std::vector<std::unique_ptr<Label>>& labels,
                       Gui* gui,
                       std::function<bool()> using_dbu,
                       std::function<bool()> using_poly_decomp_view,
                       std::function<bool()> show_ruler_as_euclidian,
                       std::function<bool()> default_mouse_wheel_zoom,
                       std::function<int()> arrow_keys_scroll_step,
                       QWidget* parent)
    : QTabWidget(parent),
      options_(options),
      output_widget_(output_widget),
      selected_(selected),
      highlighted_(highlighted),
      rulers_(rulers),
      labels_(labels),
      gui_(gui),
      using_dbu_(std::move(using_dbu)),
      using_poly_decomp_view_(std::move(using_poly_decomp_view)),
      show_ruler_as_euclidian_(std::move(show_ruler_as_euclidian)),
      default_mouse_wheel_zoom_(std::move(default_mouse_wheel_zoom)),
      arrow_keys_scroll_step_(std::move(arrow_keys_scroll_step)),
      logger_(nullptr)
{
  setTabBarAutoHide(true);
  connect(this, &QTabWidget::currentChanged, this, &LayoutTabs::tabChange);
}

void LayoutTabs::updateBackgroundColors()
{
  for (LayoutViewer* viewer : viewers_) {
    updateBackgroundColor(viewer);
    viewer->fullRepaint();
  }
}

void LayoutTabs::updateBackgroundColor(LayoutViewer* viewer)
{
  QPalette palette;
  palette.setColor(QPalette::Window, options_->background());
  viewer->setPalette(palette);
  viewer->setAutoFillBackground(true);
}

void LayoutTabs::chipLoaded(odb::dbChip* chip)
{
  // Check if we already have a tab for this block
  for (LayoutViewer* viewer : viewers_) {
    if (viewer->getChip() == chip) {
      return;
    }
  }

  populateModuleColors(chip->getBlock());
  auto viewer = new LayoutViewer(options_,
                                 output_widget_,
                                 selected_,
                                 highlighted_,
                                 rulers_,
                                 labels_,
                                 modules_,
                                 focus_nets_,
                                 route_guides_,
                                 net_tracks_,
                                 gui_,
                                 using_dbu_,
                                 show_ruler_as_euclidian_,
                                 using_poly_decomp_view_,
                                 this);
  viewer->setLogger(logger_);
  viewers_.push_back(viewer);
  if (command_executing_) {
    viewer->commandAboutToExecute();
  }
  auto scroll = new LayoutScroll(
      viewer, default_mouse_wheel_zoom_, arrow_keys_scroll_step_, this);
  viewer->chipLoaded(chip);

  auto tech = chip->getTech();
  const auto name
      = fmt::format("{} ({})", chip->getName(), tech ? tech->getName() : "-");
  addTab(scroll, name.c_str());

  updateBackgroundColor(viewer);

  // forward signals from the viewer upward
  connect(viewer, &LayoutViewer::location, this, &LayoutTabs::location);
  connect(viewer, &LayoutViewer::selected, this, &LayoutTabs::selected);
  connect(viewer,
          qOverload<const Selected&>(&LayoutViewer::addSelected),
          this,
          qOverload<const Selected&>(&LayoutTabs::addSelected));
  connect(viewer,
          qOverload<const SelectionSet&>(&LayoutViewer::addSelected),
          this,
          qOverload<const SelectionSet&>(&LayoutTabs::addSelected));
  connect(viewer, &LayoutViewer::addRuler, this, &LayoutTabs::addRuler);
  connect(viewer,
          &LayoutViewer::focusNetsChanged,
          this,
          &LayoutTabs::focusNetsChanged);
  connect(viewer, &LayoutViewer::viewUpdated, this, &LayoutTabs::viewUpdated);

  emit newViewer(viewer);
}

void LayoutTabs::tabChange(int index)
{
  current_viewer_ = viewers_[index];

  emit setCurrentChip(current_viewer_->getChip());
}

void LayoutTabs::setLogger(utl::Logger* logger)
{
  logger_ = logger;
}

// Forwarding methods/slots downward to the current viewer

void LayoutTabs::zoomIn()
{
  if (current_viewer_) {
    if (current_viewer_->isCursorInsideViewport()) {
      const odb::Point focus = current_viewer_->screenToDBU(
          current_viewer_->mapFromGlobal(QCursor::pos()));

      current_viewer_->zoomIn(focus, true);
    } else {
      current_viewer_->zoomIn();
    }
  }
}

void LayoutTabs::zoomOut()
{
  if (current_viewer_) {
    if (current_viewer_->isCursorInsideViewport()) {
      const odb::Point focus = current_viewer_->screenToDBU(
          current_viewer_->mapFromGlobal(QCursor::pos()));

      current_viewer_->zoomOut(focus, true);
    } else {
      current_viewer_->zoomOut();
    }
  }
}

void LayoutTabs::zoomTo(const odb::Rect& rect_dbu)
{
  if (current_viewer_) {
    current_viewer_->zoomTo(rect_dbu);
  }
}

void LayoutTabs::zoomTo(const odb::Point& focus, int diameter)
{
  if (current_viewer_) {
    current_viewer_->zoomTo(focus, diameter);
  }
}

void LayoutTabs::fit()
{
  if (current_viewer_) {
    current_viewer_->fit();
  }
}

void LayoutTabs::fullRepaint()
{
  // Send this to all views.  A command or other action may cause
  // any view to need updating so be conservative and do all.
  for (auto viewer : viewers_) {
    viewer->fullRepaint();
  }
}

void LayoutTabs::startRulerBuild()
{
  if (current_viewer_) {
    current_viewer_->startRulerBuild();
  }
}

void LayoutTabs::cancelRulerBuild()
{
  if (current_viewer_) {
    current_viewer_->cancelRulerBuild();
  }
}

void LayoutTabs::selection(const Selected& selection)
{
  if (current_viewer_) {
    current_viewer_->selection(selection);
  }
}

void LayoutTabs::selectionFocus(const Selected& focus)
{
  if (current_viewer_) {
    current_viewer_->selectionFocus(focus);
  }
}

void LayoutTabs::updateModuleVisibility(odb::dbModule* module, bool visible)
{
  modules_[module].visible = visible;
  fullRepaint();
}

void LayoutTabs::updateModuleColor(odb::dbModule* module,
                                   const QColor& color,
                                   bool user_selected)
{
  modules_[module].color = color;
  if (user_selected) {
    modules_[module].user_color = color;
  }
  fullRepaint();
}

void LayoutTabs::populateModuleColors(odb::dbBlock* block)
{
  if (block == nullptr) {
    return;
  }

  ColorGenerator generator;

  for (auto* module : block->getModules()) {
    auto color = generator.getQColor();
    modules_[module] = {color, color, color, true};
  }
}

void LayoutTabs::addFocusNet(odb::dbNet* net)
{
  const auto& [itr, inserted] = focus_nets_.insert(net);
  if (inserted) {
    emit focusNetsChanged();
    fullRepaint();
  }
}

void LayoutTabs::addRouteGuides(odb::dbNet* net)
{
  const auto& [itr, inserted] = route_guides_.insert(net);
  if (inserted) {
    emit routeGuidesChanged();
    fullRepaint();
  }
}

void LayoutTabs::addNetTracks(odb::dbNet* net)
{
  const auto& [itr, inserted] = net_tracks_.insert(net);
  if (inserted) {
    emit netTracksChanged();
    fullRepaint();
  }
}

void LayoutTabs::removeFocusNet(odb::dbNet* net)
{
  if (focus_nets_.erase(net) > 0) {
    emit focusNetsChanged();
    fullRepaint();
  }
}

void LayoutTabs::removeRouteGuides(odb::dbNet* net)
{
  if (route_guides_.erase(net) > 0) {
    emit routeGuidesChanged();
    fullRepaint();
  }
}

void LayoutTabs::removeNetTracks(odb::dbNet* net)
{
  if (net_tracks_.erase(net) > 0) {
    emit netTracksChanged();
    fullRepaint();
  }
}

void LayoutTabs::clearFocusNets()
{
  if (!focus_nets_.empty()) {
    focus_nets_.clear();
    emit focusNetsChanged();
    fullRepaint();
  }
}

void LayoutTabs::clearRouteGuides()
{
  if (!route_guides_.empty()) {
    route_guides_.clear();
    emit routeGuidesChanged();
    fullRepaint();
  }
}

void LayoutTabs::clearNetTracks()
{
  if (!net_tracks_.empty()) {
    net_tracks_.clear();
    emit netTracksChanged();
    fullRepaint();
  }
}

void LayoutTabs::exit()
{
  for (auto viewer : viewers_) {
    viewer->exit();
  }
}

void LayoutTabs::commandAboutToExecute()
{
  for (LayoutViewer* viewer : viewers_) {
    viewer->commandAboutToExecute();
  }
  command_executing_ = true;
}

void LayoutTabs::commandFinishedExecuting()
{
  for (LayoutViewer* viewer : viewers_) {
    viewer->commandFinishedExecuting();
  }
  command_executing_ = false;
}

void LayoutTabs::restoreTclCommands(std::vector<std::string>& cmds)
{
  if (current_viewer_) {
    current_viewer_->restoreTclCommands(cmds);
  }
}

void LayoutTabs::executionPaused()
{
  if (current_viewer_) {
    current_viewer_->executionPaused();
  }
}

void LayoutTabs::resetCache()
{
  for (LayoutViewer* viewer : viewers_) {
    viewer->resetCache();
  }
}

}  // namespace gui
