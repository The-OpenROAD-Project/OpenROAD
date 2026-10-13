// TOP: top
// TECH: nangate45
// TARGETS: feedthrough_plus_internal_load, submodule
// CLUE: submodule input feeds both a feedthrough assign and an internal gate.

module s (input a, output y1, output y2);
  assign y1 = a;
  INV_X1 g0 (.A(a), .ZN(y2));
endmodule

module top (input i, output o1, output o2);
  s u0 (.a(i), .y1(o1), .y2(o2));
endmodule
