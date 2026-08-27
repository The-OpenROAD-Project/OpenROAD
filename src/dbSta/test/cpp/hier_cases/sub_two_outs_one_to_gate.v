// TOP: top
// TECH: nangate45
// TARGETS: alias_two_outputs, submodule, mixed_endpoints
// CLUE: child aliases two outputs; one goes straight to a top port, the other through a top gate: mixed endpoint bracket for the hier duplicate-driver assign.

module dualout (input a, output y1, output y2);
  wire w;
  INV_X1 g0 (.A(a), .ZN(w));
  assign y1 = w;
  assign y2 = w;
endmodule

module top (input i, output o1, output o2);
  wire m2;
  dualout u0 (.a(i), .y1(o1), .y2(m2));
  INV_X1 g1 (.A(m2), .ZN(o2));
endmodule
