#include <print>
#include <string_view>
#include <vector>

#include "VMockArray.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

std::vector<std::string_view> arguments(int argc, char* argv[])
{
  std::vector<std::string_view> x = {};
  for (int i = 1; i < argc; ++i) {
    auto arg = std::string_view(argv[i]);
    if (arg.starts_with("+verilator")) {
      continue;
    }
    x.push_back(arg);
  }
  return x;
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

  QData* inputs[]
      = {&top->io_ins_down_0,  &top->io_ins_down_1,  &top->io_ins_down_2,
         &top->io_ins_down_3,  &top->io_ins_down_4,  &top->io_ins_down_5,
         &top->io_ins_down_6,  &top->io_ins_down_7,  &top->io_ins_left_0,
         &top->io_ins_left_1,  &top->io_ins_left_2,  &top->io_ins_left_3,
         &top->io_ins_left_4,  &top->io_ins_left_5,  &top->io_ins_left_6,
         &top->io_ins_left_7,  &top->io_ins_right_0, &top->io_ins_right_1,
         &top->io_ins_right_2, &top->io_ins_right_3, &top->io_ins_right_4,
         &top->io_ins_right_5, &top->io_ins_right_6, &top->io_ins_right_7,
         &top->io_ins_up_0,    &top->io_ins_up_1,    &top->io_ins_up_2,
         &top->io_ins_up_3,    &top->io_ins_up_4,    &top->io_ins_up_5,
         &top->io_ins_up_6,    &top->io_ins_up_7};

  top->trace(vcd, 99);  // Trace all levels of hierarchy
  vcd->open(args.front().data());

  int tick = 0;
  for (auto input : inputs) {
    for (int i = 0; i < 5; i++) {
      if (Verilated::gotFinish()) {
        goto done;
      }
      *input = tick ^ ((tick / 2) % 2 ? 0 : 0xffffffffffffffffUL);
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
done:
  vcd->flush();
  vcd->close();

  top->final();
  delete top;
  return 0;
}
