// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "display_control_tree.h"
#include "renderer_controls.h"
#include "tile_generator.h"
#include "web/core.h"

namespace odb {
class dbDatabase;
}  // namespace odb

namespace utl {
class Logger;
}

namespace web {

// What gui:: commands act on with no Qt window: Qt-named controls that
// save_image -web draws by.  Not a backend, so Gui::enabled() stays false.
class ScriptView : public GuiBackend
{
 public:
  ScriptView(odb::dbDatabase* db, utl::Logger* logger);

  void redraw() override {}
  void pause(int /* timeout_ms */) override {}
  bool isPaused() const override { return false; }
  // A saved image with no -area frames the whole design already.
  void fit() override {}

  bool checkDisplayControlVisible(const std::string& name) override;
  bool checkDisplayControlSelectable(const std::string& name) override;
  void setDisplayControlVisible(const std::string& name, bool value) override;
  void setDisplayControlSelectable(const std::string& name,
                                   bool value) override;
  void setDisplayControlColor(const std::string& name,
                              const Painter::Color& color) override;
  void saveDisplayControls() override;
  void restoreDisplayControls() override;
  std::vector<std::string> displayControlPaths() override;
  // From the renderers, possibly on the render threads: only the shared
  // RendererControls, under its own lock.
  bool checkRendererControl(const std::string& path) override;
  void setRendererControl(const std::string& path, bool value) override;

  // Shared with the web viewer, so the browser's renderer toggles and Tcl's
  // agree.
  RendererControls& rendererControls() { return renderer_controls_; }

  // What a saved image is drawn by.
  struct Frame
  {
    TileVisibility vis;
    // Set only once a script has chosen Misc/Background's color.
    std::optional<Painter::Color> background;
  };
  // The current controls with `overrides` (TileVisibility JSON keys or
  // display-control patterns) on top, leaving the controls as they were.
  Frame frame(const std::map<std::string, bool>& overrides = {});

 private:
  // A control a pattern can name: a leaf of the tree, or a registered
  // renderer's control, whose state lives in renderer_controls_.
  struct Match
  {
    std::string path;
    DisplayControlTree::Leaf* leaf;
  };

  // Brings the design's layers and sites, and the renderers' controls, in.
  void refreshLocked();
  std::vector<Match> matchLocked(DisplayControlTree& tree,
                                 const std::string& pattern,
                                 bool selectable);
  bool check(const std::string& name, bool selectable);
  void set(const std::string& name, bool selectable, bool value);

  odb::dbDatabase* db_;
  utl::Logger* logger_;

  std::mutex mutex_;
  DisplayControlTree tree_;
  RendererControls renderer_controls_;
  std::optional<Painter::Color> background_;

  struct Saved
  {
    DisplayControlTree::Leaves leaves;
    RendererControls::Values renderer_controls;
  };
  std::optional<Saved> saved_;
};

}  // namespace web
