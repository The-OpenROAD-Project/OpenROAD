// TOP: top
// TECH: nangate45
// TARGETS: bus_slice_assign, mid_level, depth_3
// CLUE: bus-slice feedthrough assign at the MIDDLE level of a depth-3 hierarchy.

module inner (input [1:0] p, output [1:0] q);
  assign q = p;
endmodule

module mid (input [3:0] mi, output [1:0] mo);
  wire [1:0] t;
  assign t = mi[3:2];
  inner u_in (.p(t), .q(mo));
endmodule

module top (input [3:0] i, input zi, output [1:0] o, output zo);
  mid u0 (.mi(i), .mo(o));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
