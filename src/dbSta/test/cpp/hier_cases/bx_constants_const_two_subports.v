// TOP: top
// TECH: nangate45
// TARGETS: tie0, tie1, multi_instance
// CLUE: the SAME literal constant fed to input ports of TWO different
// submodule instances — reader may share or duplicate the constant net;
// flat uniquification must keep both ties.
module inv1 (input a, output y);
  NOR2_X1 g (.A1(a), .A2(1'b0), .ZN(y));
endmodule

module top (input a, output y1, output y2);
  inv1 u1 (.a(1'b0), .y(y1));
  inv1 u2 (.a(1'b1), .y(y2));
endmodule
