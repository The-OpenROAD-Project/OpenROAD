// TOP: top
// TECH: nangate45
// TARGETS: alias_output_from_output, submodule
// CLUE: submodule port-to-port assign where BOTH sides are output ports of the submodule.

module oo (input a, output y1, output y2);
  INV_X1 g0 (.A(a), .ZN(y1));
  assign y2 = y1;
endmodule

module top (input i, output o1, output o2);
  oo u0 (.a(i), .y1(o1), .y2(o2));
endmodule
