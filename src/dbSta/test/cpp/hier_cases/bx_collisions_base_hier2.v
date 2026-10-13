// TOP: top
// TECH: nangate45
// TARGETS: baseline, flat_naming_probe
// CLUE: no collision; probes what names the flat writer synthesizes for
// instances and nets inside hierarchy instance x (separator, escaping).
module subx (input a, output z);
  wire y_net;
  INV_X1 y (.A(a), .ZN(y_net));
  INV_X1 u2 (.A(y_net), .ZN(z));
endmodule

module top (input in1, output o1);
  subx x (.a(in1), .z(o1));
endmodule
