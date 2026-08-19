// TOP: top
// TECH: nangate45
// TARGETS: dangling_input, escaped_port_name, dead_cone
// CLUE: sub input \p! (escaped id) feeds a dead cone and is left empty at the parent
// via .\p! (). Escaped dangling port survival + name fidelity.
module sub (input a, input \p! , output y);
  wire pd;
  INV_X1 g1 (.A(a), .ZN(y));
  INV_X1 g2 (.A(\p! ), .ZN(pd));
endmodule
module top (input in1, output out1);
  sub u1 (.a(in1), .\p! (), .y(out1));
endmodule
