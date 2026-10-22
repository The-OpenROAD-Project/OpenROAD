// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "script_view.h"
#include "tile_generator.h"
#include "tst/nangate45_fixture.h"
#include "web/core.h"
#include "web/descriptor_registry.h"
#include "web_viewer_hook.h"

namespace web {
namespace {

constexpr const char* kSite = "Rows/FreePDK45_38x28_10R_NP_162NW_34O";

class ScriptViewTest : public tst::Nangate45Fixture
{
 protected:
  ScriptView view_{getDb(), getLogger()};
};

// A renderer that only exists to carry display controls.
class ControlsOnlyRenderer : public Renderer
{
 public:
  const char* getDisplayControlGroupName() override { return "Test Group"; }
  using Renderer::addDisplayControl;
};

// The defaults DisplayControls' constructor and addTech give the same rows.
TEST_F(ScriptViewTest, DefaultsAreTheQtOnes)
{
  EXPECT_TRUE(view_.checkDisplayControlVisible("Nets/Power"));
  EXPECT_TRUE(view_.checkDisplayControlSelectable("Nets/Power"));
  EXPECT_TRUE(view_.checkDisplayControlVisible("Instances/Physical/Fill cell"));
  EXPECT_TRUE(view_.checkDisplayControlVisible("Misc/Scale bar"));
  EXPECT_TRUE(view_.checkDisplayControlVisible("Misc/Highlight selected"));
  EXPECT_TRUE(view_.checkDisplayControlVisible("Timing Path/Data path"));
  EXPECT_FALSE(view_.checkDisplayControlVisible("Tracks/Pref"));
  EXPECT_FALSE(view_.checkDisplayControlVisible("Shape Types/Fills"));
  EXPECT_FALSE(view_.checkDisplayControlVisible("Misc/Instances/Pin Names"));
  EXPECT_FALSE(view_.checkDisplayControlVisible("Misc/Detailed view"));

  // Routing and cut layers are on; masterslice and overlap go under Other,
  // which starts off; the lib's site starts as Rows does, off.
  EXPECT_TRUE(view_.checkDisplayControlVisible("Layers/metal1"));
  EXPECT_TRUE(view_.checkDisplayControlVisible("Layers/via1"));
  EXPECT_FALSE(view_.checkDisplayControlVisible("Layers/Other/poly"));
  EXPECT_FALSE(view_.checkDisplayControlVisible("Layers/Other/OVERLAP"));
  EXPECT_FALSE(view_.checkDisplayControlVisible(kSite));
}

// Even before anything else asks, a parent with rows under it is not one.
TEST_F(ScriptViewTest, TheFirstCheckOfAParentWarns)
{
  getLogger()->redirectStringBegin();
  EXPECT_FALSE(view_.checkDisplayControlVisible("Rows"));
  EXPECT_NE(getLogger()->redirectStringEnd().find("WEB-0127"),
            std::string::npos);
}

// Only leaves are controls: a parent with rows under it is not one.
TEST_F(ScriptViewTest, ParentsWithRowsAreNotControls)
{
  const std::vector<std::string> paths = view_.displayControlPaths();
  auto has = [&](const std::string& path) {
    return std::ranges::find(paths, path) != paths.end();
  };
  EXPECT_TRUE(has("Layers/Implant")) << "no implant layers, so still a leaf";
  EXPECT_FALSE(has("Layers/Other"));
  EXPECT_FALSE(has("Rows"));
  EXPECT_FALSE(has("Nets"));
  EXPECT_FALSE(has("Misc/Background")) << "it has no visible box";
  EXPECT_TRUE(std::ranges::is_sorted(paths));

  EXPECT_THROW(view_.setDisplayControlVisible("Nets", false),
               std::runtime_error);
}

TEST_F(ScriptViewTest, WildcardsMatchAsInQt)
{
  view_.setDisplayControlVisible("*", false);
  EXPECT_FALSE(view_.checkDisplayControlVisible("Nets/Signal"));
  EXPECT_FALSE(view_.checkDisplayControlVisible("Layers/metal1"));
  EXPECT_FALSE(view_.checkDisplayControlVisible("Timing Path/Legend"));

  // Case does not matter, and '*' crosses '/'.
  view_.setDisplayControlVisible("nets/power", true);
  EXPECT_TRUE(view_.checkDisplayControlVisible("Nets/Power"));
  view_.setDisplayControlVisible("Instances/*", true);
  EXPECT_TRUE(view_.checkDisplayControlVisible(
      "Instances/StdCells/Clock tree/Clock gate"));
  EXPECT_FALSE(view_.checkDisplayControlVisible("Nets/Ground"));

  EXPECT_THROW(view_.setDisplayControlVisible("Bogus/*", true),
               std::runtime_error);
}

TEST_F(ScriptViewTest, CheckingSeveralControlsAtOnceIsFalse)
{
  EXPECT_FALSE(view_.checkDisplayControlVisible("Nets/*"));
  EXPECT_FALSE(view_.checkDisplayControlVisible("Bogus"));
}

// With nothing changed, the tile renderer draws as it does by default, except
// that the layers Qt hides start hidden.
TEST_F(ScriptViewTest, DefaultsDrawLikeTheTileRendererDefaults)
{
  const TileVisibility vis = view_.frame().vis;
  const TileVisibility defaults;
  for (const char* key : {
           "stdcells",       "macros",         "pad_input",     "phys_fill",
           "std_bufinv",     "std_sequential", "net_signal",    "net_power",
           "routing",        "routing_vias",   "special_nets",  "pins",
           "pin_markers",    "pin_names",      "access_points", "regions",
           "mfg_grid",       "gcell_grid",     "inst_names",    "inst_pins",
           "inst_pin_names", "blockages",      "fills",         "rows",
           "tracks_pref",    "detailed",       "labels",
       }) {
    EXPECT_EQ(vis.flag(key), defaults.flag(key)) << key;
  }

  EXPECT_TRUE(vis.has_visible_layers);
  EXPECT_TRUE(vis.visible_layers.contains("metal1"));
  EXPECT_TRUE(vis.visible_layers.contains("via9"));
  EXPECT_FALSE(vis.visible_layers.contains("poly"));
  EXPECT_FALSE(vis.visible_layers.contains("OVERLAP"));
}

TEST_F(ScriptViewTest, ControlsMapOntoTheTileRenderer)
{
  view_.setDisplayControlVisible("Nets/Power", false);
  view_.setDisplayControlVisible("Instances/StdCells/Sequential", false);
  view_.setDisplayControlVisible("Shape Types/Pins", false);
  view_.setDisplayControlVisible("Layers/metal2", false);
  view_.setDisplayControlVisible("Rows/*", true);
  TileVisibility vis = view_.frame().vis;
  EXPECT_FALSE(vis.net_power);
  EXPECT_TRUE(vis.net_ground);
  EXPECT_FALSE(vis.std_sequential);
  EXPECT_TRUE(vis.stdcells) << "the other std cell rows are still on";
  EXPECT_FALSE(vis.pins);
  EXPECT_FALSE(vis.pin_markers);
  EXPECT_FALSE(vis.visible_layers.contains("metal2"));
  EXPECT_TRUE(vis.rows);
  EXPECT_TRUE(vis.isSiteVisible("FreePDK45_38x28_10R_NP_162NW_34O"));

  view_.setDisplayControlVisible("Instances/StdCells/*", false);
  view_.setDisplayControlVisible("Shape Types/Routing/*", false);
  vis = view_.frame().vis;
  EXPECT_FALSE(vis.stdcells);
  EXPECT_FALSE(vis.routing);
  EXPECT_TRUE(vis.special_nets);
}

TEST_F(ScriptViewTest, RestoreBringsBackWhatWasSaved)
{
  view_.saveDisplayControls();
  view_.setDisplayControlVisible("*", false);
  view_.setDisplayControlVisible("Tracks/Pref", true);
  view_.restoreDisplayControls();
  EXPECT_TRUE(view_.checkDisplayControlVisible("Nets/Power"));
  EXPECT_TRUE(view_.checkDisplayControlVisible("Layers/metal1"));
  EXPECT_FALSE(view_.checkDisplayControlVisible("Tracks/Pref"));
}

TEST_F(ScriptViewTest, OnlyTheBackgroundColorIsKept)
{
  constexpr Painter::Color kWhite{255, 255, 255};
  view_.setDisplayControlColor("Misc/Background", kWhite);
  EXPECT_EQ(view_.frame().background, kWhite);
  EXPECT_NO_THROW(view_.setDisplayControlColor("Nets/Power", kWhite));
  EXPECT_THROW(view_.setDisplayControlColor("Bogus", kWhite),
               std::runtime_error);
}

// A renderer's controls are rows too, with its own defaults and exclusivity,
// and the web viewer's hook reads the same values.
TEST_F(ScriptViewTest, RendererControlsAreSharedWithTheHook)
{
  ControlsOnlyRenderer renderer;
  renderer.addDisplayControl("A", true, {}, {"B"});
  renderer.addDisplayControl("B", false);
  Gui::get()->registerRenderer(&renderer);

  EXPECT_TRUE(view_.checkDisplayControlVisible("Test Group/A"));
  EXPECT_FALSE(view_.checkDisplayControlVisible("Test Group/B"));
  const std::vector<std::string> paths = view_.displayControlPaths();
  EXPECT_NE(std::ranges::find(paths, "Test Group/B"), paths.end());

  view_.setDisplayControlVisible("Test Group/B", true);
  view_.setDisplayControlVisible("Test Group/A", true);
  EXPECT_FALSE(view_.checkDisplayControlVisible("Test Group/B"))
      << "turning A on turns off what it excludes";

  WebViewerHook hook;
  hook.setRendererControls(&view_.rendererControls());
  hook.setDisplayControlVisible("Test Group/A", false);
  EXPECT_FALSE(view_.checkDisplayControlVisible("Test Group/A"));

  Gui::get()->unregisterRenderer(&renderer);
}

// Only Misc/Background has a color here; a renderer's control has none either.
TEST_F(ScriptViewTest, ARendererControlsColorIsIgnored)
{
  ControlsOnlyRenderer renderer;
  renderer.addDisplayControl("A", true);
  Gui::get()->registerRenderer(&renderer);
  getLogger()->redirectStringBegin();
  view_.setDisplayControlColor("Test Group/A", Painter::kRed);
  EXPECT_NE(getLogger()->redirectStringEnd().find("WEB-0129"),
            std::string::npos);
  Gui::get()->unregisterRenderer(&renderer);
}

// A renderer's control is shared with the browser, so one image cannot set it.
TEST_F(ScriptViewTest, ARendererControlInAnImagesOptionsWarns)
{
  ControlsOnlyRenderer renderer;
  renderer.addDisplayControl("A", true);
  Gui::get()->registerRenderer(&renderer);
  getLogger()->redirectStringBegin();
  view_.frame({{"Test Group/A", false}});
  EXPECT_NE(getLogger()->redirectStringEnd().find("WEB-0135"),
            std::string::npos);
  EXPECT_TRUE(view_.checkRendererControl("Test Group/A"));
  Gui::get()->unregisterRenderer(&renderer);
}

// With no Qt window the Gui's display-control calls land here, and a script
// view is not a backend: Gui::enabled() stays false.
TEST_F(ScriptViewTest, GuiRoutesToTheScriptView)
{
  Gui* gui = Gui::get();
  GuiBackend* previous = gui->getScriptView();
  gui->setScriptView(&view_);

  gui->setDisplayControlsVisible("Nets/Power", false);
  EXPECT_FALSE(view_.checkDisplayControlVisible("Nets/Power"));
  EXPECT_FALSE(gui->checkDisplayControlsVisible("Nets/Power"));
  EXPECT_TRUE(Gui::hasViewer());
  EXPECT_FALSE(Gui::enabled());

  gui->setScriptView(previous);
}

// Renderers ask from the render threads: a path nobody registered draws, and
// setting one is no error.  The display-control commands stay strict.
TEST_F(ScriptViewTest, RendererControlsAreLenient)
{
  Gui* gui = Gui::get();
  GuiBackend* previous = gui->getScriptView();
  gui->setScriptView(&view_);

  EXPECT_TRUE(gui->checkRendererControl("Nobody/Registered"));
  EXPECT_NO_THROW(gui->setRendererControl("Nobody/Registered", false));
  EXPECT_FALSE(gui->checkRendererControl("Nobody/Registered"));
  EXPECT_THROW(gui->setDisplayControlsVisible("Bogus/Control", true),
               std::runtime_error);

  gui->setScriptView(previous);
}

// As the browser's payload with no keys of their own: std_physical and
// std_other follow a stdcells option, unless given too.
TEST_F(ScriptViewTest, StdcellsTakesThePhysicalAndOtherCells)
{
  TileVisibility vis = view_.frame({{"stdcells", false}}).vis;
  EXPECT_FALSE(vis.std_physical);
  EXPECT_FALSE(vis.std_other);
  vis = view_.frame({{"stdcells", false}, {"std_physical", true}}).vis;
  EXPECT_TRUE(vis.std_physical);
  EXPECT_FALSE(vis.std_other);
}

}  // namespace
}  // namespace web
