// TOP: top
// TECH: nangate45
// TARGETS: concat_const, depth_2
// CLUE: a constant-bearing concat used INSIDE a submodule to drive a deeper
// instance's bus port — concat constants one level down from top.
module leaf4 (input [3:0] bus, output y);
  wire n1, n2;
  AND2_X1 g1 (.A1(bus[0]), .A2(bus[1]), .ZN(n1));
  AND2_X1 g2 (.A1(bus[2]), .A2(bus[3]), .ZN(n2));
  OR2_X1 g3 (.A1(n1), .A2(n2), .ZN(y));
endmodule

module mid (input m, output y);
  leaf4 lf (.bus({1'b1, m, m, 1'b0}), .y(y));
endmodule

module top (input a, output y);
  mid md (.m(a), .y(y));
endmodule
