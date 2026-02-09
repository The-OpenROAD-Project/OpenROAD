#include <cstdlib>
#include <string>

#include "gtest/gtest.h"

// NOLINTNEXTLINE
#include "Vram_8x7.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

namespace {
class Ram_8x7TestHarness
{
 public:
  VerilatedContext ctx;
  Vram_8x7 top;
  std::unique_ptr<VerilatedVcdC> trace;

  Ram_8x7TestHarness(const std::string& vcd_file)
  {
    ctx.traceEverOn(true);
    trace = std::make_unique<VerilatedVcdC>();
    top.trace(trace.get(), 100);
    trace->open(vcd_file.c_str());
  }

  ~Ram_8x7TestHarness() = default;
  // Simulate one clock cycle
  void step()
  {
    // evaluate to pick up changes on inputs
    top.eval();
    // show registers changing on rising clock edge as well as inputs changing
    // on rising clock edge
    ctx.timeInc(1);
    trace->dump(ctx.time());

    top.R0_clk = 0;
    top.W0_clk = 0;
    top.eval();
    ctx.timeInc(1);
    trace->dump(ctx.time());

    top.R0_clk = 1;
    top.W0_clk = 1;
    top.eval();
    // Now we're ready to pick up poked inputs on rising edge
  }
};
}  // namespace

TEST(Ram_8x7Test, SimpleTest)
{
  const char* output_dir = std::getenv("TEST_UNDECLARED_OUTPUTS_DIR");
  std::string vcd_path
      = output_dir ? std::string(output_dir) + "/trace.vcd" : "trace.vcd";
  Ram_8x7TestHarness harness(vcd_path);

  harness.step();
  harness.step();
  harness.step();

  // unit-test for combinational read ram_8x7 in a loop
  for (int i = 0; i < 32; ++i) {
    harness.top.W0_en = 1;
    harness.top.W0_addr = (i + 1) % 8;
    harness.top.W0_data = i + 100;  // Arbitrary data

    // Set up combinational read
    harness.top.R0_en = 1;
    harness.top.R0_addr = i % 8;
    harness.step();
    harness.top.W0_en = 0;
    harness.top.R0_en = 0;

    if (i > 0) {
      // Check read data from previous write, note that we need to mask the
      // upper bit as we're 7-bit wide
      EXPECT_EQ(harness.top.R0_data % 128, ((i - 1) + 100) % 128);
      // FIXME should always fail, but passes in Verilator source code, but
      // fails in after synthesis. EXPECT_EQ(harness.top.R0_data, ((i - 1) +
      // 100));
    }
  }
  harness.trace->close();
}
