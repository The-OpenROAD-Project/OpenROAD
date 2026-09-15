// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#include <stdlib.h>  // NOLINT(modernize-deprecated-headers): for mkdtemp()

#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <string>

#include "ClaimFile.h"
#include "gtest/gtest.h"

namespace wmk {
namespace {
namespace fs = std::filesystem;

class ClaimFileTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    std::string pattern
        = (fs::temp_directory_path() / "wmk-claims-test-XXXXXX").string();
    ASSERT_NE(mkdtemp(pattern.data()), nullptr);
    directory_ = pattern;
    destination_ = directory_ / "claims.csv";
    std::ofstream(destination_) << "old claims\n";
  }

  void TearDown() override
  {
    if (!directory_.empty()) {
      fs::remove_all(directory_);
    }
  }

  std::string read() const
  {
    std::ifstream in(destination_);
    return {std::istreambuf_iterator<char>(in),
            std::istreambuf_iterator<char>()};
  }

  void expectNoTemporaryFiles() const
  {
    for (const fs::directory_entry& entry :
         fs::directory_iterator(directory_)) {
      EXPECT_FALSE(
          entry.path().filename().string().starts_with(".wmk-claims-"));
    }
  }

  fs::path directory_;
  fs::path destination_;
};

TEST_F(ClaimFileTest, PublishReplacesCompleteFileAndPreservesPermissions)
{
  fs::permissions(destination_, fs::perms::owner_read | fs::perms::owner_write);
  {
    ClaimFile output(destination_.string());
    EXPECT_EQ(read(), "old claims\n");
    output.publish([&](std::ostream& out) {
      out << "new claims\n";
      EXPECT_EQ(read(), "old claims\n");
    });
    EXPECT_EQ(read(), "new claims\n");
    EXPECT_EQ(fs::status(destination_).permissions(),
              fs::perms::owner_read | fs::perms::owner_write);
  }
  expectNoTemporaryFiles();
}

TEST_F(ClaimFileTest, PublishCreatesNewFile)
{
  fs::remove(destination_);
  {
    ClaimFile output(destination_.string());
    EXPECT_FALSE(fs::exists(destination_));
    output.publish([](std::ostream& out) { out << "new claims\n"; });
    EXPECT_EQ(read(), "new claims\n");
  }
  expectNoTemporaryFiles();
}

TEST_F(ClaimFileTest, AbandonDoesNotPublish)
{
  {
    ClaimFile output(destination_.string());
  }
  EXPECT_EQ(read(), "old claims\n");
  expectNoTemporaryFiles();
}

TEST_F(ClaimFileTest, WriterExceptionDoesNotPublishPartialClaims)
{
  {
    ClaimFile output(destination_.string());
    EXPECT_THROW(output.publish([](std::ostream& out) {
      out << "partial claims\n";
      throw std::runtime_error("writer failed");
    }),
                 std::runtime_error);
  }
  EXPECT_EQ(read(), "old claims\n");
  expectNoTemporaryFiles();
}

TEST_F(ClaimFileTest, FailedStreamDoesNotPublish)
{
  {
    ClaimFile output(destination_.string());
    EXPECT_THROW(output.publish([](std::ostream& out) {
      out << "partial claims\n";
      out.setstate(std::ios::badbit);
    }),
                 std::runtime_error);
  }
  EXPECT_EQ(read(), "old claims\n");
  expectNoTemporaryFiles();
}

TEST_F(ClaimFileTest, DestinationChangedBeforePublicationIsRejected)
{
  {
    ClaimFile output(destination_.string());
    fs::remove(destination_);
    fs::create_directory(destination_);
    EXPECT_THROW(
        output.publish([](std::ostream& out) { out << "new claims\n"; }),
        std::runtime_error);
    EXPECT_TRUE(fs::is_directory(destination_));
  }
  expectNoTemporaryFiles();
}

TEST_F(ClaimFileTest, InvalidDestinationIsRejectedBeforeWriting)
{
  EXPECT_THROW(ClaimFile{(directory_ / "missing" / "claims.csv").string()},
               std::runtime_error);
  EXPECT_THROW(ClaimFile{directory_.string()}, std::runtime_error);
  EXPECT_THROW(ClaimFile{""}, std::runtime_error);
  const fs::path link = directory_ / "link";
  fs::create_symlink(destination_, link);
  EXPECT_THROW(ClaimFile{link.string()}, std::runtime_error);
  EXPECT_TRUE(fs::is_symlink(link));
  EXPECT_EQ(read(), "old claims\n");
  expectNoTemporaryFiles();
}

}  // namespace
}  // namespace wmk
