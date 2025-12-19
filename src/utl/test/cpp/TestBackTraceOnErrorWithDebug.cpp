#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "utl/Logger.h"

namespace utl {
namespace {

// Initialize absl symbolizer just for this test
static bool stack_trace_init = []() {
  // On Linux, this falls back to /proc/self/exe, may fail on other platforms
  absl::InitializeSymbolizer(nullptr);
  return true;
}();

TEST(ErrorFunctionTest, PrintsSymbolizedStackTrace)
{
  Logger logger;
  logger.setDebugLevel(GPL, "updateGrad", 1);

  testing::internal::CaptureStdout();
  try {
    logger.error(GPL, 73, "Test error");
  } catch (...) {
  }
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_THAT(output, testing::HasSubstr("Test error"));
  // Check that the name of this function is in the output
  EXPECT_THAT(output, testing::HasSubstr("ErrorFunctionTest"));
}

TEST(ErrorFunctionTest, PrintsNoStackTrace)
{
  Logger logger;
  testing::internal::CaptureStdout();
  try {
    logger.error(GPL, 73, "Test error");
  } catch (...) {
  }
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_THAT(output, testing::HasSubstr("Test error"));
  EXPECT_THAT(output, testing::Not(testing::HasSubstr("ErrorFunctionTest")));
}
}  // namespace
}  // namespace utl