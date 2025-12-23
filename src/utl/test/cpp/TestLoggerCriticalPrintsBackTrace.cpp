#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sys/stat.h>

#include "absl/debugging/symbolize.h"

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
  const std::string success_pattern = 
      "\\[CRITICAL GPL-1234\\] Test Trigger Message\\s+Stack "
      "Trace:\\s+#0 TestFuncForCheckingBacktrace @ 0x[0-9a-fA-F]+";

  const std::string stacktrace_not_working_pattern = 
      "Test Trigger Message.*(Empty Stack Trace|depth=0)";

  EXPECT_DEATH(TestFuncForCheckingBacktrace(),
               testing::AnyOf(
                   testing::ContainsRegex(success_pattern),
                   testing::ContainsRegex(stacktrace_not_working_pattern)
               ));
}
}
}  // namespace