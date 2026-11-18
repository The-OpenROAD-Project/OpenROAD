// TOP: top
// TECH: nangate45
// TARGETS: needs_escape, leading_digit, bus_port, writePortDcls
// CLUE: writePorts/writePortDcls spell the port with portVerilogName -> staToVerilog2,
// CLUE: same alnum-only test, and then print the bus range separately.  A digit-leading
// CLUE: escaped BUS port should yield "input [1:0] 1p;" plus bare "1p[0]" references.
module top (\1p , z);
  input [1:0] \1p ;
  output [1:0] z;
  INV_X1 g0 (.A(\1p [0]), .ZN(z[0]));
  INV_X1 g1 (.A(\1p [1]), .ZN(z[1]));
endmodule
