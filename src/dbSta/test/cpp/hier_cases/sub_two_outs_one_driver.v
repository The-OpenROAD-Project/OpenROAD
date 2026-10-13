// TOP: top
// TECH: nangate45
// TARGETS: alias_two_outputs, submodule
// CLUE: submodule whose two output ports are aliased to one internal driver.

module dualout (input a, output y1, output y2);
  wire w;
  INV_X1 g0 (.A(a), .ZN(w));
  assign y1 = w;
  assign y2 = w;
endmodule

module top (input i, output o1, output o2);
  dualout u0 (.a(i), .y1(o1), .y2(o2));
endmodule
