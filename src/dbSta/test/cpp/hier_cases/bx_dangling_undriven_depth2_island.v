// TOP: top
// TECH: nangate45
// TARGETS: undriven_net_in_module, depth_2, name_leak_bracket
// CLUE: undriven-net dead island sits in leaf at depth 2. If the hier writer
// leaks instance paths into module-local names, expect \u_m/u_l/und inside leaf.
module leaf (input a, output y);
  wire und;
  wire d;
  INV_X1 g1 (.A(a), .ZN(y));
  INV_X1 g2 (.A(und), .ZN(d));
endmodule
module mid (input a, output y);
  leaf u_l (.a(a), .y(y));
endmodule
module top (input in1, output out1);
  mid u_m (.a(in1), .y(out1));
endmodule
