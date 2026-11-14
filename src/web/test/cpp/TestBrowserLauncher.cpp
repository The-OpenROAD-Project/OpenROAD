// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <stdlib.h>  // NOLINT(modernize-deprecated-headers): for mkdtemp

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

#include "browser_launcher.h"
#include "gtest/gtest.h"

namespace web {
namespace {

// A directory of its own per test, so that no file from another run or user
// can get in the way.
class BrowserLauncherTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    std::string dir = ::testing::TempDir() + "browser_launcher_XXXXXX";
    ASSERT_NE(mkdtemp(dir.data()), nullptr);
    dir_ = dir + "/";
  }

  void TearDown() override
  {
    std::error_code ignored;
    std::filesystem::remove_all(dir_, ignored);
  }

  // An executable /bin/sh script `name` in the test's directory.
  std::string writeScript(const std::string& name,
                          const std::string& body) const
  {
    const std::string path = dir_ + name;
    std::ofstream(path) << "#!/bin/sh\n" << body;
    std::filesystem::permissions(path, std::filesystem::perms::owner_all);
    return path;
  }

  static std::string fileText(const std::string& path)
  {
    std::ifstream in(path);
    return {std::istreambuf_iterator<char>(in),
            std::istreambuf_iterator<char>()};
  }

  std::string dir_;
};

#ifdef __linux__
// A shell would end the quoting at the ' and run the touch.
TEST_F(BrowserLauncherTest, RunsNoShell)
{
  const std::string record = dir_ + "record";
  const std::string marker = dir_ + "pwned";
  // "own" is a session of its own, away from openroad's terminal.
  const std::string launcher = writeScript(
      "launcher",
      "session=shared\n"
      "[ \"$(cut -d' ' -f6 /proc/$$/stat)\" = \"$$\" ] && session=own\n"
      "printf '%s\\n' \"$#\" \"$1\" \"$session\" \"$(readlink /proc/$$/fd/0)\""
      " \"$(readlink /proc/$$/fd/1)\" > '"
          + record + "'\n");
  const std::string url
      = "http://localhost:1/?ticket=x'; touch '" + marker + "'; echo '";

  ASSERT_EQ(runBrowserLauncher({launcher, url}, "/dev/null"), "");

  EXPECT_EQ(fileText(record), "1\n" + url + "\nown\n/dev/null\n/dev/null\n");
  EXPECT_FALSE(std::filesystem::exists(marker));
}
#endif

TEST_F(BrowserLauncherTest, ReportsFailures)
{
  const std::string missing = dir_ + "no_such_launcher";
  EXPECT_EQ(runBrowserLauncher({missing, "x"}, "/dev/null"),
            missing + ": No such file or directory");

  const std::string failing
      = writeScript("exit3", "echo 'no browser here' >&2\nexit 3\n");
  const std::string errfile = dir_ + "exit3.err";
  // Longer than what the script writes, so an untruncated file shows it.
  std::ofstream(errfile) << "stale text from an earlier launch\n";
  EXPECT_EQ(runBrowserLauncher({failing}, errfile),
            failing + " exited with status 3");
  EXPECT_EQ(fileText(errfile), "no browser here\n");

  const std::string killed = writeScript("kill9", "kill -9 $$\n");
  EXPECT_EQ(runBrowserLauncher({killed}, "/dev/null"),
            killed + " was killed by signal 9");
}

// The error file only explains a failure; it must not cause one.
TEST_F(BrowserLauncherTest, AnUnopenableErrorFileStillLaunches)
{
  const std::string launcher = writeScript("ok", "exit 0\n");
  EXPECT_EQ(runBrowserLauncher({launcher}, dir_ + "no/such/dir/err"), "");
}

// As xdg-open does when it runs the browser itself, in the foreground.
TEST_F(BrowserLauncherTest, StopsWaitingForALauncherThatStays)
{
  const std::string launcher = writeScript("stays", "exec sleep 3\n");
  const auto start = std::chrono::steady_clock::now();
  EXPECT_EQ(runBrowserLauncher(
                {launcher}, "/dev/null", std::chrono::milliseconds(10)),
            "");
  EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(2));
}

}  // namespace
}  // namespace web
