#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "spdlog/sinks/ostream_sink.h"
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
  EXPECT_DEATH(TestFuncForCheckingBacktrace(),
               "\\[CRITICAL GPL-1234\\] Test Trigger Message\\s+Stack "
               "Trace:\\s+#0 TestFuncForCheckingBacktrace @ 0x[0-9a-fA-F]+");
}
}  // namespace
}  // namespace utl