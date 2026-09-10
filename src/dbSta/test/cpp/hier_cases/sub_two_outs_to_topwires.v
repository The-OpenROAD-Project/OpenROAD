// TOP: top
// TECH: nangate45
// TARGETS: alias_two_outputs, submodule, internal_top_wires
// CLUE: child aliases two of its outputs, but at top they land on internal WIRES feeding gates instead of top ports: does the spurious hier assign still appear?

module dualout (input a, output y1, output y2);
  wire w;
  INV_X1 g0 (.A(a), .ZN(w));
  assign y1 = w;
  assign y2 = w;
endmodule

module top (input i, output o1, output o2);
  wire m1, m2;
  dualout u0 (.a(i), .y1(m1), .y2(m2));
  INV_X1 g1 (.A(m1), .ZN(o1));
  INV_X1 g2 (.A(m2), .ZN(o2));
endmodule
