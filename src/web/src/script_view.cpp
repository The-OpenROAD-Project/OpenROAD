// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "script_view.h"

#include <algorithm>
#include <map>
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
      }
    }
  }

  Frame frame;
  tree.apply(frame.vis);
  frame.vis.setFlags(flags);
  frame.background = background_;
  return frame;
}

}  // namespace web
