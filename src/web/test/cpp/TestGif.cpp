// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "tst/nangate45_fixture.h"
#include "web/web.h"

namespace web {
namespace {

class GifTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    block_->setDieArea(odb::Rect(0, 0, 20000, 20000));
    for (int i = 0; i < 8; ++i) {
      odb::dbMaster* master = lib_->findMaster("BUF_X16");
      ASSERT_NE(master, nullptr);
      odb::dbInst* inst = odb::dbInst::create(
          block_, master, ("buf" + std::to_string(i)).c_str());
      inst->setLocation(2000 + i * 1800, 2000 + i * 1800);
      inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    }
  }

  void TearDown() override
  {
    for (const auto& path : output_files_) {
      std::filesystem::remove(path);
    }
  }

  std::string tempGif(const std::string& label)
  {
    std::string path
        = std::filesystem::temp_directory_path()
          / ("web_gif_" + label + "_" + std::to_string(::getpid()) + ".gif");
    output_files_.push_back(path);
    return path;
  }

  // GIF files start with the 6-byte "GIF89a" signature.
  static bool hasGifSignature(const std::string& path)
  {
    std::ifstream f(path, std::ios::binary);
    char sig[6] = {0};
    f.read(sig, 6);
    return f.gcount() == 6 && std::string(sig, 6) == "GIF89a";
  }

  WebServer makeServer()
  {
    return WebServer(getDb(), nullptr, getLogger(), nullptr);
  }

  std::vector<std::string> output_files_;
};

TEST_F(GifTest, StartAddEndProducesValidGif)
{
  const std::string path = tempGif("basic");
  WebServer server = makeServer();

  const int key = server.gifStart(path);
  EXPECT_EQ(key, 0);
  const odb::Rect area(0, 0, 0, 0);  // zero → whole die + margin
  server.gifAddFrame(key, area, /*width_px=*/200, 0, /*delay=*/5, "");
  server.gifAddFrame(key, area, /*width_px=*/200, 0, /*delay=*/5, "");
  server.gifEnd(key);

  ASSERT_TRUE(std::filesystem::exists(path));
  EXPECT_GT(std::filesystem::file_size(path), 0u);
  EXPECT_TRUE(hasGifSignature(path));
}

TEST_F(GifTest, LaterFramesWithDifferentSizeAreScaled)
{
  const std::string path = tempGif("scaled");
  WebServer server = makeServer();

  const int key = server.gifStart(path);
  const odb::Rect area(0, 0, 0, 0);
  server.gifAddFrame(key, area, /*width_px=*/200, 0, 5, "");
  // Different width → must be resampled to the first frame's dimensions.
  server.gifAddFrame(key, area, /*width_px=*/400, 0, 5, "");
  server.gifEnd(key);

  ASSERT_TRUE(std::filesystem::exists(path));
  EXPECT_TRUE(hasGifSignature(path));
}

TEST_F(GifTest, EndWithoutFramesWritesNoFile)
{
  const std::string path = tempGif("empty");
  WebServer server = makeServer();

  const int key = server.gifStart(path);
  server.gifEnd(key);  // no frames added

  // The encoder only opens the file on the first frame, so nothing is written.
  EXPECT_FALSE(std::filesystem::exists(path));
}

TEST_F(GifTest, MultipleConcurrentStreamsGetDistinctKeys)
{
  WebServer server = makeServer();
  const std::string a = tempGif("streamA");
  const std::string b = tempGif("streamB");

  const int ka = server.gifStart(a);
  const int kb = server.gifStart(b);
  EXPECT_NE(ka, kb);

  const odb::Rect area(0, 0, 0, 0);
  server.gifAddFrame(ka, area, 150, 0, 5, "");
  server.gifAddFrame(kb, area, 150, 0, 5, "");
  server.gifEnd(ka);
  server.gifEnd(kb);

  EXPECT_TRUE(hasGifSignature(a));
  EXPECT_TRUE(hasGifSignature(b));
}

}  // namespace
}  // namespace web
