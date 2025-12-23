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
    logger.error(GPL, 9973, "Test error");
  } catch (...) {
  }
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_THAT(output, testing::HasSubstr("Test error"));
  // Check that the name of this function is in the output or that
  // stackunwinding tried to trigger but isn't working for this binary.
  EXPECT_THAT(output,
              testing::AnyOf(testing::HasSubstr("ErrorFunctionTest"),
                             testing::HasSubstr("Stack unwind failed")));
}

TEST(ErrorFunctionTest, PrintsNoStackTrace)
{
  Logger logger;
  testing::internal::CaptureStdout();
  try {
    logger.error(GPL, 9967, "Test error");
  } catch (...) {
  }
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_THAT(output, testing::HasSubstr("Test error"));
  EXPECT_THAT(
      output,
      testing::AllOf(testing::Not(testing::HasSubstr("ErrorFunctionTest")),
                     testing::Not(testing::HasSubstr("Stack unwind failed"))));
}
}  // namespace
}  // namespace utl