// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "script_view.h"

#include <algorithm>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "db_sta/dbSta.hh"
#include "display_control_tree.h"
#include "odb/db.h"
#include "renderer_controls.h"
#include "sta/Clock.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "tile_generator.h"
#include "timing_path_painter.h"
#include "timing_report.h"
#include "utl/Logger.h"
#include "web/clockTree.h"
#include "web/core.h"
#include "web/heatMap.h"
#include "web/timing_path_style.h"

namespace web {

namespace {

// TimingPathRenderer's controls, which the Qt gui lists whether or not its
// timing widget is open, and the part of the path each one draws.
constexpr std::pair<const char*, bool TimingPathParts::*> kTimingPathRows[] = {
    {kTimingPathDataRow, &TimingPathParts::data},
    {kTimingPathLaunchClockRow, &TimingPathParts::launch_clock},
    {kTimingPathCaptureClockRow, &TimingPathParts::capture_clock},
    {kTimingPathLegendRow, &TimingPathParts::legend},
};

std::string timingPathRow(const char* name)
{
  return std::string(kTimingPathGroup) + "/" + name;
}

constexpr const char* kBackground = "Misc/Background";

const char* columnName(const bool selectable)
{
  return selectable ? "select" : "visible";
}

}  // namespace

ScriptView::ScriptView(odb::dbDatabase* db,
                       sta::dbSta* sta,
                       utl::Logger* logger)
    : db_(db), sta_(sta), logger_(logger)
{
  for (const auto& [name, part] : kTimingPathRows) {
    tree_.addLeaf(timingPathRow(name),
                  {.visible = true, .selectable = std::nullopt});
  }
}

void ScriptView::refreshLocked()
{
  tree_.addDesignRows(db_);
  seedRendererControls(renderer_controls_);
}

std::vector<ScriptView::Match> ScriptView::matchLocked(
    DisplayControlTree& tree,
    const std::string& pattern,
    const bool selectable)
{
  std::vector<Match> matches;
  for (const auto& [path, leaf] : tree.leaves()) {
    const bool has_box
        = selectable ? leaf.selectable.has_value() : leaf.visible.has_value();
    if (has_box && DisplayControlTree::matches(pattern, path)) {
      matches.push_back({.path = path, .leaf = tree.find(path)});
    }
  }
  if (selectable) {
    // Qt gives a renderer's controls no selectable box.
    return matches;
  }

  std::set<std::string> renderer_paths;
  for (Renderer* renderer : Gui::get()->renderers()) {
    for (const auto& [name, control] : renderer->getDisplayControls()) {
      const std::string path = renderer->displayControlPath(name);
      if (tree.find(path) == nullptr
          && DisplayControlTree::matches(pattern, path)) {
        renderer_paths.insert(path);
      }
    }
  }
  for (const std::string& path : renderer_paths) {
    matches.push_back({.path = path, .leaf = nullptr});
  }
  return matches;
}

bool ScriptView::check(const std::string& name, const bool selectable)
{
  const std::scoped_lock lock(mutex_);
  refreshLocked();
  const std::vector<Match> matches = matchLocked(tree_, name, selectable);
  if (matches.empty()) {
    logger_->warn(utl::WEB,
                  127,
                  "Unable to find {} display control at {}.",
                  columnName(selectable),
                  name);
    return false;
  }
  if (matches.size() > 1) {
    logger_->warn(utl::WEB,
                  128,
                  "Found {} controls matching {} at {}.",
                  matches.size(),
                  name,
                  columnName(selectable));
    return false;
  }
  const Match& match = matches.front();
  if (match.leaf == nullptr) {
    return renderer_controls_.check(match.path);
  }
  const auto& value = selectable ? match.leaf->selectable : match.leaf->visible;
  return value.value_or(false);
}

void ScriptView::set(const std::string& name,
                     const bool selectable,
                     const bool value)
{
  const std::scoped_lock lock(mutex_);
  refreshLocked();
  const std::vector<Match> matches = matchLocked(tree_, name, selectable);
  if (matches.empty()) {
    logger_->error(utl::WEB,
                   126,
                   "Unable to find {} display control at {}.",
                   columnName(selectable),
                   name);
  }
  for (const Match& match : matches) {
    if (match.leaf != nullptr) {
      (selectable ? match.leaf->selectable : match.leaf->visible) = value;
    } else {
      web::setRendererControl(renderer_controls_, match.path, value);
    }
  }
}

bool ScriptView::checkDisplayControlVisible(const std::string& name)
{
  return check(name, /*selectable=*/false);
}

bool ScriptView::checkDisplayControlSelectable(const std::string& name)
{
  return check(name, /*selectable=*/true);
}

void ScriptView::setDisplayControlVisible(const std::string& name,
                                          const bool value)
{
  set(name, /*selectable=*/false, value);
}

void ScriptView::setDisplayControlSelectable(const std::string& name,
                                             const bool value)
{
  set(name, /*selectable=*/true, value);
}

void ScriptView::setDisplayControlColor(const std::string& name,
                                        const Painter::Color& color)
{
  const std::scoped_lock lock(mutex_);
  refreshLocked();
  bool found = !matchLocked(tree_, name, /*selectable=*/false).empty();
  bool ignored = false;
  for (const auto& [path, leaf] : tree_.leaves()) {
    if (!DisplayControlTree::matches(name, path)) {
      continue;
    }
    found = true;
    if (path == kBackground) {
      background_ = color;
    } else {
      ignored = true;
    }
  }
  if (!found) {
    logger_->error(utl::WEB, 130, "Unable to find {} display control", name);
  }
  if (ignored) {
    logger_->warn(utl::WEB,
                  129,
                  "Without the Qt GUI only the {} color is used; ignoring it "
                  "for {}.",
                  kBackground,
                  name);
  }
}

void ScriptView::saveDisplayControls()
{
  const std::scoped_lock lock(mutex_);
  refreshLocked();
  saved_ = Saved{
      .leaves = tree_.leaves(),
      .renderer_controls = renderer_controls_.values(),
  };
}

void ScriptView::restoreDisplayControls()
{
  const std::scoped_lock lock(mutex_);
  if (!saved_) {
    return;
  }
  // Only the controls that still exist, as DisplayControls::restore does.
  for (const auto& [path, leaf] : saved_->leaves) {
    if (DisplayControlTree::Leaf* current = tree_.find(path)) {
      *current = leaf;
    }
  }
  renderer_controls_.restore(saved_->renderer_controls);
}

bool ScriptView::checkRendererControl(const std::string& path)
{
  seedRendererControls(renderer_controls_);
  return renderer_controls_.check(path);
}

void ScriptView::setRendererControl(const std::string& path, const bool value)
{
  seedRendererControls(renderer_controls_);
  web::setRendererControl(renderer_controls_, path, value);
}

std::vector<std::string> ScriptView::displayControlPaths()
{
  const std::scoped_lock lock(mutex_);
  refreshLocked();
  std::vector<std::string> paths;
  for (const Match& match : matchLocked(tree_, "*", /*selectable=*/false)) {
    paths.push_back(match.path);
  }
  std::ranges::sort(paths);
  return paths;
}

ScriptView::Frame ScriptView::frame(
    const std::map<std::string, bool>& overrides)
{
  const std::scoped_lock lock(mutex_);
  refreshLocked();

  // The patterns go on a copy, so an image's options do not stick.
  DisplayControlTree tree = tree_;
  std::map<std::string, bool> flags;
  std::map<std::string, bool> renderer_overrides;
  for (const auto& [key, value] : overrides) {
    if (TileVisibility::isFlag(key)) {
      flags.emplace(key, value);
      continue;
    }
    const std::vector<Match> matches
        = matchLocked(tree, key, /*selectable=*/false);
    if (matches.empty()) {
      logger_->error(utl::WEB,
                     131,
                     "{} is neither a display control nor a display option.",
                     key);
    }
    for (const Match& match : matches) {
      if (match.leaf != nullptr) {
        match.leaf->visible = value;
      } else {
        renderer_overrides[match.path] = value;
      }
    }
  }

  Frame frame;
  tree.apply(frame.vis);
  frame.vis.setFlags(flags);
  frame.background = background_;
  frame.heat_map = activeHeatMapLocked(renderer_overrides);
  if (frame.heat_map != nullptr && frame.heat_map->getShortName() == "RUDY") {
    // Drawn as the heat map, so not again by the tiles' own RUDY layer.
    frame.vis.rudy = false;
  }
  showHeatMapLocked(frame.heat_map);
  watchBlockLocked();
  const DisplayControlTree::Leaf* highlight_selected
      = tree.find("Misc/Highlight selected");
  if (highlight_selected != nullptr
      && highlight_selected->visible.value_or(false)) {
    frame.selection = selection_;
  }
  frame.highlights = highlights_;
  frame.timing_path = timing_path_;
  for (const auto& [name, part] : kTimingPathRows) {
    const DisplayControlTree::Leaf* leaf = tree.find(timingPathRow(name));
    frame.timing_path_parts.*part
        = leaf != nullptr && leaf->visible.value_or(false);
  }
  const DisplayControlTree::Leaf* clock_trees = tree.find(kClockTreesControl);
  if (clock_viewer_shown_ && clock_trees != nullptr
      && clock_trees->visible.value_or(false)) {
    frame.clock_tree = buildClockTreeLocked(clock_viewer_clock_);
    const auto depth = clock_tree_depths_.find(clock_viewer_clock_);
    if (depth != clock_tree_depths_.end()) {
      frame.clock_tree_depth = depth->second;
    }
  }
  odb::dbChip* chip = db_ != nullptr ? db_->getChip() : nullptr;
  if (!marker_category_.empty() && chip != nullptr) {
    frame.markers = chip->findMarkerCategory(marker_category_.c_str());
  }
  return frame;
}

void ScriptView::selectMarkers(odb::dbMarkerCategory* markers)
{
  const std::scoped_lock lock(mutex_);
  // The top category, as DRCWidget::selectCategory shows; by name, so one
  // destroyed since is simply not found.
  marker_category_
      = markers != nullptr ? markers->getTopCategory()->getName() : "";
}

HeatMapDataSource* ScriptView::activeHeatMapLocked(
    const std::map<std::string, bool>& overrides)
{
  // By name, so the answer does not depend on the set's pointer order; the
  // controls exclude each other, so normally one at most is on.
  std::vector<HeatMapDataSource*> sources(Gui::get()->getHeatMaps().begin(),
                                          Gui::get()->getHeatMaps().end());
  std::ranges::sort(sources, {}, &HeatMapDataSource::getName);
  for (HeatMapDataSource* source : sources) {
    const std::string path
        = std::string(kHeatMapGroup) + "/" + source->getName();
    const auto it = overrides.find(path);
    const bool on = it != overrides.end()
                        ? it->second
                        : renderer_controls_.contains(path)
                              && renderer_controls_.check(path);
    if (on) {
      return source;
    }
  }
  return nullptr;
}

void ScriptView::showHeatMapLocked(HeatMapDataSource* heat_map)
{
  const std::string name = heat_map != nullptr ? heat_map->getName() : "";
  if (name == shown_heat_map_) {
    return;
  }
  for (HeatMapDataSource* source : Gui::get()->getHeatMaps()) {
    if (source->getName() == shown_heat_map_) {
      source->hide();
    }
  }
  if (heat_map != nullptr) {
    heat_map->ensureMap();
    heat_map->show();
  }
  shown_heat_map_ = name;
}

void ScriptView::watchBlockLocked()
{
  // Every chip's block: in 3DBlox what is selected can sit in any chiplet.
  std::vector<odb::dbChip*> chips;
  std::vector<odb::dbBlock*> blocks;
  if (db_ != nullptr) {
    for (odb::dbChip* chip : db_->getChips()) {
      chips.push_back(chip);
      if (odb::dbBlock* block = chip->getBlock()) {
        blocks.push_back(block);
      }
    }
  }
  auto owned = [](const auto& watcher) { return watcher->hasOwner(); };
  if (chips == watched_chips_ && blocks == watched_
      && std::ranges::all_of(marker_watchers_, owned)
      && std::ranges::all_of(watchers_, owned)) {
    return;
  }
  // Another design, or a watched one destroyed: nothing selected is left.
  forgetLocked();
  marker_watchers_.clear();
  watchers_.clear();
  watched_chips_ = chips;
  watched_ = blocks;
  for (odb::dbChip* chip : chips) {
    auto watcher = std::make_unique<MarkerWatcher>(this);
    watcher->addOwner(chip);
    marker_watchers_.push_back(std::move(watcher));
  }
  for (odb::dbBlock* block : blocks) {
    auto watcher = std::make_unique<BlockWatcher>(this);
    watcher->addOwner(block);
    watchers_.push_back(std::move(watcher));
  }
}

void ScriptView::forget()
{
  const std::scoped_lock lock(mutex_);
  forgetLocked();
}

void ScriptView::forgetLocked()
{
  selection_.clear();
  for (SelectionSet& group : highlights_) {
    group.clear();
  }
  timing_path_.reset();
}

void ScriptView::showWorstTimingPath(const bool setup)
{
  // An STA error goes to the caller and, as in TimingWidget, leaves no path.
  clearTimingPath();
  std::optional<TimingPathSummary> path;
  if (sta_ != nullptr) {
    path = TimingReport(sta_).getWorstPath(setup);
  }
  const std::scoped_lock lock(mutex_);
  watchBlockLocked();
  timing_path_ = std::move(path);
}

void ScriptView::clearTimingPath()
{
  const std::scoped_lock lock(mutex_);
  timing_path_.reset();
}

void ScriptView::showWidget(const std::string& name, const bool show)
{
  // Of the docks, only the clock tree viewer draws on the layout.
  if (name != kClockTreeViewerTitle && name != kClockTreeViewerName) {
    return;
  }
  const std::scoped_lock lock(mutex_);
  if (show == clock_viewer_shown_) {
    return;
  }
  clock_viewer_shown_ = show;
  showClockTreesRowLocked(show && !clock_viewer_clock_.empty());
}

void ScriptView::selectClockviewerClock(const std::string& clock_name,
                                        const std::optional<int> depth)
{
  const std::scoped_lock lock(mutex_);
  watchBlockLocked();
  selectClockviewerClockLocked(clock_name, depth);
}

void ScriptView::selectClockviewerClockLocked(const std::string& clock_name,
                                              const std::optional<int> depth)
{
  // As ClockWidget::selectClock: the viewer is shown, and populated on first
  // use with the first clock current, before the clock is looked for.
  const bool was_shown = clock_viewer_shown_;
  clock_viewer_shown_ = true;
  const std::vector<std::string> names = clockNamesLocked();
  std::string current = clock_viewer_clock_;
  if (!clock_viewer_populated_) {
    clock_viewer_populated_ = true;
    clock_tree_depths_.clear();
    current = names.empty() ? "" : names.front();
  }
  const bool found = std::ranges::find(names, clock_name) != names.end();
  if (found) {
    current = clock_name;
    if (depth) {
      clock_tree_depths_[clock_name] = *depth;
    }
  }
  if (!was_shown || current != clock_viewer_clock_) {
    showClockTreesRowLocked(!current.empty());
  }
  clock_viewer_clock_ = current;
  if (!found) {
    logger_->error(utl::WEB, 133, "Unable to find clock: {}", clock_name);
  }
}

void ScriptView::saveClockTreeImage(const std::string& clock_name,
                                    const std::string& filename,
                                    const std::string& scene,
                                    const std::optional<int> width_px,
                                    const std::optional<int> height_px)
{
  std::shared_ptr<const ClockTree> tree;
  {
    const std::scoped_lock lock(mutex_);
    watchBlockLocked();
    const bool was_shown = clock_viewer_shown_;
    sta::Scene* current = clockViewerSceneLocked();
    if (!scene.empty() && (current == nullptr || scene != current->name())) {
      if (sta_ == nullptr || sta_->findScene(scene) == nullptr) {
        logger_->error(utl::WEB, 134, "Unable to find \"{}\" scene", scene);
      }
      // Another scene: ClockWidget populates again, for that scene.
      clock_viewer_scene_ = scene;
      clock_viewer_populated_ = false;
    }
    selectClockviewerClockLocked(clock_name, std::nullopt);
    tree = buildClockTreeLocked(clock_viewer_clock_);
    if (!was_shown) {
      clock_viewer_shown_ = false;
      showClockTreesRowLocked(false);
    }
  }
  if (tree == nullptr || !clock_tree_image_writer_) {
    return;
  }
  // ClockWidget sizes the image as the viewer is; a script view has no
  // viewer, so a size not given is save_image -web's default width.
  constexpr int kDefaultSize = 1024;
  clock_tree_image_writer_(*tree,
                           filename,
                           width_px.value_or(kDefaultSize),
                           height_px.value_or(kDefaultSize));
}

void ScriptView::showClockTreesRowLocked(const bool show)
{
  tree_.removeLeaf(kClockTreesControl);
  if (show) {
    tree_.addLeaf(kClockTreesControl,
                  {.visible = true, .selectable = std::nullopt});
  }
}

sta::Scene* ScriptView::clockViewerSceneLocked() const
{
  if (sta_ == nullptr) {
    return nullptr;
  }
  if (sta::Scene* scene = sta_->findScene(clock_viewer_scene_)) {
    return scene;
  }
  return sta_->scenes().empty() ? nullptr : sta_->scenes().front();
}

std::vector<std::string> ScriptView::clockNamesLocked() const
{
  std::vector<std::string> names;
  if (sta::Scene* scene = clockViewerSceneLocked()) {
    for (sta::Clock* clock : scene->sdc()->clocks()) {
      // A virtual clock, or one on a pin with no net, has no tree to show.
      if (ClockTree(clock, sta_->getDbNetwork()).getNet() != nullptr) {
        names.push_back(clock->name());
      }
    }
  }
  return names;
}

std::shared_ptr<const ClockTree> ScriptView::buildClockTreeLocked(
    const std::string& clock_name) const
{
  sta::Scene* scene = clockViewerSceneLocked();
  const sta::Clock* clock
      = scene != nullptr ? scene->sdc()->findClock(clock_name) : nullptr;
  if (clock == nullptr) {
    return nullptr;
  }
  for (auto& tree : buildClockTrees(sta_, scene, clock)) {
    if (tree->getNet() != nullptr) {
      return std::move(tree);
    }
  }
  return nullptr;
}

void ScriptView::setSelected(const Selected& selection)
{
  const std::scoped_lock lock(mutex_);
  watchBlockLocked();
  selection_.clear();
  if (selection) {
    selection_.insert(selection);
  }
}

void ScriptView::addSelected(const Selected& selection)
{
  const std::scoped_lock lock(mutex_);
  watchBlockLocked();
  if (selection) {
    selection_.insert(selection);
  }
}

void ScriptView::addSelected(const SelectionSet& selection,
                             const bool /* find_in_cts */)
{
  const std::scoped_lock lock(mutex_);
  watchBlockLocked();
  for (const Selected& selected : selection) {
    if (selected) {
      selection_.insert(selected);
    }
  }
}

void ScriptView::removeSelectedByType(const std::string& type)
{
  const std::scoped_lock lock(mutex_);
  std::erase_if(selection_, [&type](const Selected& selected) {
    return selected.getTypeName() == type;
  });
}

void ScriptView::addHighlighted(const SelectionSet& selection,
                                const int highlight_group)
{
  // No dialog to ask for a group, so a negative one is ignored.
  if (highlight_group < 0 || highlight_group >= kNumHighlightSet) {
    return;
  }
  const std::scoped_lock lock(mutex_);
  watchBlockLocked();
  for (const Selected& selected : selection) {
    if (selected) {
      highlights_[highlight_group].insert(selected);
    }
  }
}

void ScriptView::clearHighlighted(const int highlight_group)
{
  const std::scoped_lock lock(mutex_);
  if (highlight_group < 0) {
    for (SelectionSet& group : highlights_) {
      group.clear();
    }
  } else if (highlight_group < kNumHighlightSet) {
    highlights_[highlight_group].clear();
  }
}

}  // namespace web
