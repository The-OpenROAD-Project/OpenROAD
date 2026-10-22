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

#include "display_control_tree.h"
#include "odb/db.h"
#include "renderer_controls.h"
#include "tile_generator.h"
#include "utl/Logger.h"
#include "web/core.h"

namespace web {

namespace {

// TimingPathRenderer's controls (staGui.cpp), which the Qt gui lists whether
// or not its timing widget is open.
constexpr const char* kTimingPathRows[] = {
    "Timing Path/Data path",
    "Timing Path/Launch clock",
    "Timing Path/Capture clock",
    "Timing Path/Legend",
};

constexpr const char* kBackground = "Misc/Background";

const char* columnName(const bool selectable)
{
  return selectable ? "select" : "visible";
}

}  // namespace

ScriptView::ScriptView(odb::dbDatabase* db, utl::Logger* logger)
    : db_(db), logger_(logger)
{
  for (const char* path : kTimingPathRows) {
    tree_.addLeaf(path, {.visible = true, .selectable = std::nullopt});
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
  const std::vector<Match> matches
      = matchLocked(tree_, name, /*selectable=*/false);
  bool found = !matches.empty();
  // A renderer's control has no color here.
  bool ignored = std::ranges::any_of(
      matches, [](const Match& match) { return match.leaf == nullptr; });
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
    bool renderer_matched = false;
    for (const Match& match : matches) {
      if (match.leaf != nullptr) {
        match.leaf->visible = value;
      } else {
        renderer_matched = true;
      }
    }
    if (renderer_matched) {
      logger_->warn(utl::WEB,
                    135,
                    "Renderer controls matching {} cannot be set for one "
                    "image; ignoring them.",
                    key);
    }
  }

  Frame frame;
  tree.apply(frame.vis);
  frame.vis.setFlags(flags);
  frame.background = background_;
  watchBlockLocked();
  const DisplayControlTree::Leaf* highlight_selected
      = tree.find("Misc/Highlight selected");
  if (highlight_selected != nullptr
      && highlight_selected->visible.value_or(false)) {
    frame.selection = selection_;
  }
  frame.highlights = highlights_;
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
