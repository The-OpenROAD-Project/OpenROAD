// TOP: top
// TECH: nangate45
// TARGETS: alias_output_from_output, depth_3
// CLUE: the two-output alias is created at the LEAF of a depth-3 hierarchy and carried up through a mid level to two top outputs.

module leaf (input a, output y1, output y2);
  INV_X1 g0 (.A(a), .ZN(y1));
  assign y2 = y1;
endmodule

module mid (input ma, output my1, output my2);
  leaf u_l (.a(ma), .y1(my1), .y2(my2));
endmodule

module top (input i, output o1, output o2);
  mid u0 (.ma(i), .my1(o1), .my2(o2));
endmodule
