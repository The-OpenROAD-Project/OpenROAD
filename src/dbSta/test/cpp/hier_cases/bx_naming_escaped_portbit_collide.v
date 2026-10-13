// TOP: top
// TECH: nangate45
// TARGETS: escaped_port, busbit_lookalike, real_bus_collision, depth_1
// CLUE: scalar top port \d[1]  coexists with real bus port input [1:0] d;
// boundary naming must keep them distinct or LEC sees a boundary mismatch.
module top (input [1:0] d, input \d[1] , output z, output y);
  XOR2_X1 x1 (.A(d[1]), .B(d[0]), .Z(z));
  INV_X1 g1 (.A(\d[1] ), .ZN(y));
endmodule
