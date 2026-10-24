// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "display_control_tree.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbChipCallBackObj.h"
#include "renderer_controls.h"
#include "tile_generator.h"
#include "web/core.h"

namespace odb {
class dbBlock;
class dbBlockage;
class dbBPin;
class dbBTerm;
class dbChip;
class dbDatabase;
class dbInst;
class dbITerm;
class dbMarker;
class dbMarkerCategory;
class dbModBTerm;
class dbModInst;
class dbModITerm;
class dbModNet;
class dbModule;
class dbNet;
class dbObstruction;
class dbRegion;
class dbRow;
class dbSWire;
class dbWire;
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

  // The selection and highlight groups, by MainWindow's rules.
  void setSelected(const Selected& selection) override;
  void addSelected(const Selected& selection) override;
  void addSelected(const SelectionSet& selection, bool find_in_cts) override;
  void removeSelectedByType(const std::string& type) override;
  void addHighlighted(const SelectionSet& selection,
                      int highlight_group) override;
  void clearHighlighted(int highlight_group) override;

  // Shared with the web viewer, so the browser's renderer toggles and Tcl's
  // agree.
  RendererControls& rendererControls() { return renderer_controls_; }

  // What a saved image is drawn by.
  struct Frame
  {
    TileVisibility vis;
    // Set only once a script has chosen Misc/Background's color.
    std::optional<Painter::Color> background;
    // Empty when Misc/Highlight selected is off.
    SelectionSet selection;
    HighlightSet highlights;
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
  // A destroyed object would leave a dangling Selected, so destroying anything
  // that can be selected drops the selection and the highlights.
  class BlockWatcher : public odb::dbBlockCallBackObj
  {
   public:
    explicit BlockWatcher(ScriptView* view) : view_(view) {}
    void inDbInstDestroy(odb::dbInst* /* inst */) override { view_->forget(); }
    void inDbModInstDestroy(odb::dbModInst* /* inst */) override
    {
      view_->forget();
    }
    void inDbModuleDestroy(odb::dbModule* /* module */) override
    {
      view_->forget();
    }
    void inDbNetDestroy(odb::dbNet* /* net */) override { view_->forget(); }
    void inDbModNetDestroy(odb::dbModNet* /* net */) override
    {
      view_->forget();
    }
    void inDbITermDestroy(odb::dbITerm* /* iterm */) override
    {
      view_->forget();
    }
    void inDbModITermDestroy(odb::dbModITerm* /* iterm */) override
    {
      view_->forget();
    }
    void inDbBTermDestroy(odb::dbBTerm* /* bterm */) override
    {
      view_->forget();
    }
    void inDbModBTermDestroy(odb::dbModBTerm* /* bterm */) override
    {
      view_->forget();
    }
    void inDbBPinDestroy(odb::dbBPin* /* pin */) override { view_->forget(); }
    void inDbBlockageDestroy(odb::dbBlockage* /* blockage */) override
    {
      view_->forget();
    }
    void inDbObstructionDestroy(odb::dbObstruction* /* obs */) override
    {
      view_->forget();
    }
    void inDbRegionDestroy(odb::dbRegion* /* region */) override
    {
      view_->forget();
    }
    void inDbRowDestroy(odb::dbRow* /* row */) override { view_->forget(); }
    void inDbWireDestroy(odb::dbWire* /* wire */) override { view_->forget(); }
    void inDbSWireDestroy(odb::dbSWire* /* wire */) override
    {
      view_->forget();
    }

   private:
    ScriptView* view_;
  };
  // The markers live on the chip, not on its block.
  class MarkerWatcher : public odb::dbChipCallBackObj
  {
   public:
    explicit MarkerWatcher(ScriptView* view) : view_(view) {}
    void inDbMarkerDestroy(odb::dbMarker* /* marker */) override
    {
      view_->forget();
    }
    void inDbMarkerCategoryDestroy(
        odb::dbMarkerCategory* /* category */) override
    {
      view_->forget();
    }

   private:
    ScriptView* view_;
  };

  // Watches every chip and its block, dropping a selection made in another
  // design.
  void watchBlockLocked();
  void forget();
  void forgetLocked();
  void set(const std::string& name, bool selectable, bool value);

  odb::dbDatabase* db_;
  utl::Logger* logger_;

  std::mutex mutex_;
  DisplayControlTree tree_;
  RendererControls renderer_controls_;
  std::optional<Painter::Color> background_;
  SelectionSet selection_;
  HighlightSet highlights_;
  std::vector<odb::dbChip*> watched_chips_;
  std::vector<odb::dbBlock*> watched_;
  std::vector<std::unique_ptr<MarkerWatcher>> marker_watchers_;
  std::vector<std::unique_ptr<BlockWatcher>> watchers_;

  struct Saved
  {
    DisplayControlTree::Leaves leaves;
    RendererControls::Values renderer_controls;
  };
  std::optional<Saved> saved_;
};

}  // namespace web
