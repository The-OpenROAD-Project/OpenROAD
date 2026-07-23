// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "tst/nangate45_fixture.h"
#include "web/web.h"

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
      = R"({"version":1,"cookies":{"or_bg_color":"%23202020"}})";
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

// ─── Restore ────────────────────────────────────────────────────────────────

TEST_F(SaveDisplayControlsTest, RestoreValidJsonSucceeds)
{
  WebServer server(getDb(), /*sta=*/nullptr, getLogger(), /*interp=*/nullptr);
  server.initLogger();

  const std::string path = tempJson("valid");
  writeFile(path, R"({"version":1,"cookies":{"or_show_dbu":"1"}})");

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

// ─── Round-trip ─────────────────────────────────────────────────────────────

TEST_F(SaveDisplayControlsTest, SaveThenRestoreRoundTrip)
{
  WebServer server(getDb(), /*sta=*/nullptr, getLogger(), /*interp=*/nullptr);
  server.initLogger();

  const std::string state
      = R"({"version":1,"cookies":{"or_visibility":"%7B%22rows%22%3Atrue%7D"}})";
  server.setDisplayState(state);

  const std::string path = tempJson("roundtrip");
  server.saveDisplayControls(path);
  ASSERT_TRUE(std::filesystem::exists(path));
  EXPECT_EQ(readFile(path), state);

  // The saved file is valid JSON and restores without throwing.
  EXPECT_NO_THROW(server.restoreDisplayControls(path));
}

}  // namespace
}  // namespace web
