#include <print>
#include <string_view>
#include <vector>

#include "VMockArray.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

std::vector<std::string_view> arguments(int argc, char* argv[])
{
  std::vector<std::string_view> filtered_args = {};
  for (int i = 1; i < argc; ++i) {
    auto arg = std::string_view(argv[i]);
    if (arg.starts_with("+verilator")) {
      continue;
    }
    filtered_args.push_back(arg);
  }
  return filtered_args;
}

// Write a 64-bit value into a specific 64-bit slot of a wide bus.
// Yosys flattens unpacked arrays into wide packed buses, so
// io_ins_down[N] becomes io_ins_down[N*64-1:0] with element i at
// bits [i*64+63 : i*64].
static void write_slot(WData* bus, int slot, QData value)
{
  int word = slot * 2;  // 64 bits = 2 x 32-bit words
  bus[word] = static_cast<IData>(value);
  bus[word + 1] = static_cast<IData>(value >> 32);
}

int main(int argc, char** argv)
{
  Verilated::commandArgs(argc, argv);

  auto args = arguments(argc, argv);
  if (args.size() != 1) {
    std::print(stderr, "Missing argument!\n");
    return -EINVAL;
  }

  VMockArray* top = new VMockArray;

  Verilated::traceEverOn(true);
  auto* vcd = new VerilatedVcdC;

  top->reset = 1;
  top->clock = 0;

  top->trace(vcd, 99);  // Trace all levels of hierarchy
  vcd->open(args.front().data());

  int tick = 0;

  // Stimulate column buses (down/up), then row buses (left/right)
  struct
  {
    WData* bus1;
    WData* bus2;
    int count;
  } stimuli[] = {
      {top->io_ins_down.data(), top->io_ins_up.data(), ARRAY_COLS},
      {top->io_ins_left.data(), top->io_ins_right.data(), ARRAY_ROWS},
  };

  for (auto& s : stimuli) {
    for (int idx = 0; idx < s.count; idx++) {
      for (int i = 0; i < 5; i++) {
        if (Verilated::gotFinish()) {
          goto done;
        }
        QData value = tick ^ ((tick / 2) % 2 ? 0 : 0xffffffffffffffffUL);
        write_slot(s.bus1, idx, value);
        write_slot(s.bus2, idx, value);
        if (tick == 9) {
          top->reset = 0;
        }
        for (int k = 0; k < 2; k++) {
          top->eval();
          vcd->dump(tick++ * 125);
          top->clock = !top->clock;
        }
      }
    }
  }
done:
  vcd->flush();
  vcd->close();

  top->final();
  delete vcd;
  delete top;
  return 0;
}
