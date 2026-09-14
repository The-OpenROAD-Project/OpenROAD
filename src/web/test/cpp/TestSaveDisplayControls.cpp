// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "boost/json.hpp"
#include "gtest/gtest.h"
#include "gui/gui.h"
#include "gui/heatMap.h"
#include "tst/nangate45_fixture.h"
#include "web/web.h"
#include "web_viewer_hook.h"

namespace web {
namespace {

// ─── Fixture ────────────────────────────────────────────────────────────────

class SaveDisplayControlsTest : public tst::Nangate45Fixture
{
 protected:
  void TearDown() override
  {
    for (const auto& path : output_files_) {
      std::filesystem::remove(path);
    }
  }

  std::string tempJson(const std::string& label)
  {
    std::string path
        = std::filesystem::temp_directory_path()
          / ("web_dc_" + label + "_" + std::to_string(::getpid()) + ".json");
    output_files_.push_back(path);
    return path;
  }

  static std::string readFile(const std::string& path)
  {
    std::ifstream f(path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
  }

  static void writeFile(const std::string& path, const std::string& content)
  {
    std::ofstream f(path);
    f << content;
  }

  std::vector<std::string> output_files_;
};

// ─── Save ─────────────────────────────────────────────────────────────────

TEST_F(SaveDisplayControlsTest, SavesStateToFileVerbatim)
{
  WebServer server(getDb(), /*sta=*/nullptr, getLogger(), /*interp=*/nullptr);
  server.initLogger();  // creates the viewer hook that holds the cache

  const std::string state
      = R"({"version":1,"entries":{"or_bg_color":"%23202020"}})";
  server.setDisplayState(state);

  const std::string path = tempJson("save");
  server.saveDisplayControls(path);

  ASSERT_TRUE(std::filesystem::exists(path));
  EXPECT_EQ(readFile(path), state);
}

TEST_F(SaveDisplayControlsTest, SaveWithoutStateWritesNothing)
{
  WebServer server(getDb(), /*sta=*/nullptr, getLogger(), /*interp=*/nullptr);
  server.initLogger();  // hook exists but no client ever synced a state

  const std::string path = tempJson("empty");
  server.saveDisplayControls(path);

  EXPECT_FALSE(std::filesystem::exists(path));
}

TEST_F(SaveDisplayControlsTest, SaveWithoutServerThrows)
{
  WebServer server(getDb(), /*sta=*/nullptr, getLogger(), /*interp=*/nullptr);
  // No initLogger()/serve(): the viewer hook was never created.  logger_->
  // error() throws, surfacing a clear "server not running" error to Tcl.
  const std::string path = tempJson("noserver");
  EXPECT_ANY_THROW(server.saveDisplayControls(path));
  EXPECT_FALSE(std::filesystem::exists(path));
}

// Opening the target succeeding says nothing about the write: reporting success
// after a failed one would leave a truncated file behind.  /dev/full accepts
// the open and fails every write with ENOSPC, which is the cheapest way to
// reach that path without filling a disk.
TEST_F(SaveDisplayControlsTest, SaveReportsWriteFailure)
{
  if (!std::filesystem::exists("/dev/full")) {
    GTEST_SKIP() << "/dev/full is Linux-specific";
  }
  WebServer server(getDb(), /*sta=*/nullptr, getLogger(), /*interp=*/nullptr);
  server.initLogger();
  server.setDisplayState(R"({"version":1,"entries":{"or_show_dbu":"1"}})");

  EXPECT_ANY_THROW(server.saveDisplayControls("/dev/full"));
}

// ─── Restore ────────────────────────────────────────────────────────────────

TEST_F(SaveDisplayControlsTest, RestoreValidJsonSucceeds)
{
  WebServer server(getDb(), /*sta=*/nullptr, getLogger(), /*interp=*/nullptr);
  server.initLogger();

  const std::string path = tempJson("valid");
  writeFile(path, R"({"version":1,"entries":{"or_show_dbu":"1"}})");

  // No clients connected: the broadcast is a no-op but must not throw.
  EXPECT_NO_THROW(server.restoreDisplayControls(path));
}

TEST_F(SaveDisplayControlsTest, RestoreMissingFileThrows)
{
  WebServer server(getDb(), /*sta=*/nullptr, getLogger(), /*interp=*/nullptr);
  server.initLogger();

  // A missing file surfaces a clear Tcl error (logger_->error throws).
  EXPECT_ANY_THROW(
      server.restoreDisplayControls("/nonexistent/web_dc_missing.json"));
}

TEST_F(SaveDisplayControlsTest, RestoreInvalidJsonThrows)
{
  WebServer server(getDb(), /*sta=*/nullptr, getLogger(), /*interp=*/nullptr);
  server.initLogger();

  const std::string path = tempJson("invalid");
  writeFile(path, "{ this is not valid json ");

  EXPECT_ANY_THROW(server.restoreDisplayControls(path));
}

// The client writes these entries straight into document.cookie and Web
// Storage, so the shape is validated here, at the file boundary, and a bad file
// is rejected loudly instead of half-applying.

TEST_F(SaveDisplayControlsTest, RestoreRejectsMissingEntriesObject)
{
  WebServer server(getDb(), /*sta=*/nullptr, getLogger(), /*interp=*/nullptr);
  server.initLogger();

  const std::string path = tempJson("no_entries");
  writeFile(path, R"({"version":1})");

  EXPECT_ANY_THROW(server.restoreDisplayControls(path));
}

TEST_F(SaveDisplayControlsTest, RestoreRejectsNonStringEntry)
{
  WebServer server(getDb(), /*sta=*/nullptr, getLogger(), /*interp=*/nullptr);
  server.initLogger();

  // A JSON number is not interchangeable with the string the readers compare
  // against ("1" === "1"), so coercing it would silently change the setting.
  const std::string path = tempJson("number_entry");
  writeFile(path, R"({"version":1,"entries":{"or_show_dbu":1}})");

  EXPECT_ANY_THROW(server.restoreDisplayControls(path));
}

TEST_F(SaveDisplayControlsTest, RestoreRejectsCookieDelimiterInValue)
{
  WebServer server(getDb(), /*sta=*/nullptr, getLogger(), /*interp=*/nullptr);
  server.initLogger();

  // A ';' would let the file forge attributes on the cookie the client writes.
  const std::string path = tempJson("cookie_inject");
  writeFile(
      path,
      R"({"version":1,"entries":{"or_bg_color":"#fff; Domain=evil.test"}})");

  EXPECT_ANY_THROW(server.restoreDisplayControls(path));
}

// ─── Round-trip ─────────────────────────────────────────────────────────────

TEST_F(SaveDisplayControlsTest, SaveThenRestoreRoundTrip)
{
  WebServer server(getDb(), /*sta=*/nullptr, getLogger(), /*interp=*/nullptr);
  server.initLogger();

  const std::string state
      = R"({"version":1,"entries":{"or_visibility":"%7B%22rows%22%3Atrue%7D"}})";
  server.setDisplayState(state);

  const std::string path = tempJson("roundtrip");
  server.saveDisplayControls(path);
  ASSERT_TRUE(std::filesystem::exists(path));
  EXPECT_EQ(readFile(path), state);

  // The saved file is valid JSON and restores without throwing.
  EXPECT_NO_THROW(server.restoreDisplayControls(path));
}

// ─── Per-renderer control exclusivity ───────────────────────────────────────
//
// Qt scopes mutual exclusivity to the parent group and treats "" as "every
// sibling" (DisplayControls::itemChanged).  Renderers that share a group name
// share the parent, so a sibling can belong to a different renderer.

namespace {

// A renderer that only exists to carry display controls.
class ControlsOnlyRenderer : public gui::Renderer
{
 public:
  ControlsOnlyRenderer(const char* group) : group_(group) {}
  const char* getDisplayControlGroupName() override { return group_; }

  // Exposed so the tests can build the control set they need.
  using gui::Renderer::addDisplayControl;

 private:
  const char* group_;
};

}  // namespace

TEST(RendererControlExclusivity, TurningOneOnTurnsOffTheNamedSiblings)
{
  ControlsOnlyRenderer renderer("Detailed Router");
  renderer.addDisplayControl("Maze search", true, {}, {"Graph edges"});
  renderer.addDisplayControl("Graph edges", true);
  renderer.addDisplayControl("Route guides", true);
  gui::Gui::get()->registerRenderer(&renderer);

  WebViewerHook hook;
  for (const char* name : {"Maze search", "Graph edges", "Route guides"}) {
    hook.setDisplayControlVisible(std::string("Detailed Router/") + name, true);
  }
  applyRendererControlExclusivity(&hook, "Detailed Router/Maze search");

  EXPECT_TRUE(hook.checkDisplayControlVisible("Detailed Router/Maze search"))
      << "the control that was switched on stays on";
  EXPECT_FALSE(hook.checkDisplayControlVisible("Detailed Router/Graph edges"))
      << "the named sibling goes off";
  EXPECT_TRUE(hook.checkDisplayControlVisible("Detailed Router/Route guides"))
      << "an unnamed sibling is untouched";

  gui::Gui::get()->unregisterRenderer(&renderer);
}

TEST(RendererControlExclusivity, EmptyNameExcludesEverySibling)
{
  ControlsOnlyRenderer renderer("IR Drop");
  renderer.addDisplayControl("Shapes", true, {}, {""});
  renderer.addDisplayControl("Nodes", true);
  renderer.addDisplayControl("Sources", true);
  gui::Gui::get()->registerRenderer(&renderer);

  WebViewerHook hook;
  for (const char* name : {"Shapes", "Nodes", "Sources"}) {
    hook.setDisplayControlVisible(std::string("IR Drop/") + name, true);
  }
  applyRendererControlExclusivity(&hook, "IR Drop/Shapes");

  EXPECT_TRUE(hook.checkDisplayControlVisible("IR Drop/Shapes"));
  EXPECT_FALSE(hook.checkDisplayControlVisible("IR Drop/Nodes"));
  EXPECT_FALSE(hook.checkDisplayControlVisible("IR Drop/Sources"));

  gui::Gui::get()->unregisterRenderer(&renderer);
}

TEST(RendererControlExclusivity, DoesNotReachIntoAnotherGroup)
{
  ControlsOnlyRenderer router("Detailed Router");
  router.addDisplayControl("Maze search", true, {}, {""});
  ControlsOnlyRenderer pdn("PDN");
  pdn.addDisplayControl("Vias", true);
  gui::Gui::get()->registerRenderer(&router);
  gui::Gui::get()->registerRenderer(&pdn);

  WebViewerHook hook;
  hook.setDisplayControlVisible("Detailed Router/Maze search", true);
  hook.setDisplayControlVisible("PDN/Vias", true);
  applyRendererControlExclusivity(&hook, "Detailed Router/Maze search");

  EXPECT_TRUE(hook.checkDisplayControlVisible("PDN/Vias"))
      << "exclusivity is scoped to the parent group";

  gui::Gui::get()->unregisterRenderer(&router);
  gui::Gui::get()->unregisterRenderer(&pdn);
}

// A control with no exclusivity set changes nothing else.
TEST(RendererControlExclusivity, NoExclusivityIsANoOp)
{
  ControlsOnlyRenderer renderer("PDN");
  renderer.addDisplayControl("Vias", true);
  renderer.addDisplayControl("Straps", true);
  gui::Gui::get()->registerRenderer(&renderer);

  WebViewerHook hook;
  hook.setDisplayControlVisible("PDN/Vias", true);
  hook.setDisplayControlVisible("PDN/Straps", true);
  applyRendererControlExclusivity(&hook, "PDN/Vias");

  EXPECT_TRUE(hook.checkDisplayControlVisible("PDN/Straps"));

  gui::Gui::get()->unregisterRenderer(&renderer);
}

// ─── The served list of per-renderer controls ────────────────────────────────

TEST(RendererControlsJson, ListsARegisteredRenderersControls)
{
  ControlsOnlyRenderer renderer("PDN");
  renderer.addDisplayControl("Vias", false);
  renderer.addDisplayControl("Straps", true);
  gui::Gui::get()->registerRenderer(&renderer);

  WebViewerHook hook;
  const boost::json::value parsed
      = boost::json::parse(rendererControlsJson(&hook));
  const boost::json::array& controls
      = parsed.as_object().at("controls").as_array();

  std::map<std::string, bool> seen;
  for (const boost::json::value& entry : controls) {
    const boost::json::object& o = entry.as_object();
    if (std::string(o.at("group").as_string()) == "PDN") {
      seen[std::string(o.at("path").as_string())] = o.at("visible").as_bool();
    }
  }
  EXPECT_EQ(seen.size(), 2u);
  EXPECT_FALSE(seen.at("PDN/Vias")) << "the renderer's own default is served";
  EXPECT_TRUE(seen.at("PDN/Straps"));

  gui::Gui::get()->unregisterRenderer(&renderer);
}

// The web draws heat maps through its own tile layer and its own panel group,
// so the HeatMapRenderer's copy would be a second, settings-less "Heat Maps"
// group in the same panel.  It must not be listed — but it must still be
// seeded to its default of off, which is what stops the heat map from being
// drawn a second time through the renderer path.
TEST(RendererControlsJson, OmitsHeatMapControlsButStillSeedsThem)
{
  gui::registerBuiltinHeatMapSources(/*sta=*/nullptr, nullptr);
  const auto& sources = gui::getRegisteredHeatMapSources();
  ASSERT_FALSE(sources.empty()) << "registerBuiltinHeatMapSources ran";
  const std::string source_name = sources.front()->getName();

  // Stand in for the HeatMapRenderer that gui::Gui::registerHeatMap creates in
  // the real binary: same group, and a control named after the source, off by
  // default (HeatMapRenderer passes initial_visible = false).
  ControlsOnlyRenderer renderer("Heat Maps");
  renderer.addDisplayControl(source_name, false);
  gui::Gui::get()->registerRenderer(&renderer);

  WebViewerHook hook;
  const boost::json::value parsed
      = boost::json::parse(rendererControlsJson(&hook));
  for (const boost::json::value& entry :
       parsed.as_object().at("controls").as_array()) {
    EXPECT_NE(std::string(entry.as_object().at("name").as_string()),
              source_name)
        << "a heat map source must not appear as a renderer control";
  }

  EXPECT_FALSE(hook.checkDisplayControlVisible("Heat Maps/" + source_name))
      << "it is still seeded off, so the renderer path draws nothing";

  gui::Gui::get()->unregisterRenderer(&renderer);
}

}  // namespace

// The filter needs both halves: a control merely NAMED like a heat map, in
// some other renderer's group, is a real control and must still be listed.
TEST(RendererControlsJson, KeepsAHeatMapNameThatBelongsToAnotherGroup)
{
  gui::registerBuiltinHeatMapSources(/*sta=*/nullptr, nullptr);
  const auto& sources = gui::getRegisteredHeatMapSources();
  ASSERT_FALSE(sources.empty());
  const std::string source_name = sources.front()->getName();

  ControlsOnlyRenderer renderer("Detailed Router");
  renderer.addDisplayControl(source_name, true);
  gui::Gui::get()->registerRenderer(&renderer);

  WebViewerHook hook;
  const boost::json::value parsed
      = boost::json::parse(rendererControlsJson(&hook));
  bool found = false;
  for (const boost::json::value& entry :
       parsed.as_object().at("controls").as_array()) {
    const boost::json::object& o = entry.as_object();
    if (std::string(o.at("group").as_string()) == "Detailed Router"
        && std::string(o.at("name").as_string()) == source_name) {
      found = true;
    }
  }
  EXPECT_TRUE(found) << "only the heat map renderer's own group is filtered";

  gui::Gui::get()->unregisterRenderer(&renderer);
}

}  // namespace web
