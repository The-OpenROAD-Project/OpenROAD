// TOP: top
// TECH: nangate45
// TARGETS: bus_slice_assign, depth_3, leaf_level
// CLUE: finding-2 bus-slice feedthrough assign sitting at the deepest LEAF of a depth-3 hierarchy, with plain buses carrying it up.

module leaf (input [3:0] la, output [1:0] ly);
  assign ly = la[3:2];
endmodule

module mid (input [3:0] ma, output [1:0] my);
  leaf u_l (.la(ma), .ly(my));
endmodule

module top (input [3:0] i, input zi, output [1:0] o, output zo);
  mid u0 (.ma(i), .my(o));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
