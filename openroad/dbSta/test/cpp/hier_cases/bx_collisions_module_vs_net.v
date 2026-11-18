// TOP: top
// TECH: nangate45
// TARGETS: same_scope_names, module_vs_net
// CLUE: module named x (definitions namespace) while top has a NET named x;
// legal per LRM since the namespaces differ -- probes reader/writer.
module x (input a, output z);
  BUF_X1 u1 (.A(a), .Z(z));
endmodule

module top (input in1, output o1, output o2);
  wire x;
  x u1 (.a(in1), .z(x));
  INV_X1 g2 (.A(x), .ZN(o1));
  BUF_X1 g3 (.A(x), .Z(o2));
endmodule
