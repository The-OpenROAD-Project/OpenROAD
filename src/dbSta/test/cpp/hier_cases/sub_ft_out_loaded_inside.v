// TOP: top
// TECH: nangate45
// TARGETS: feedthrough_net_internal_load, submodule
// CLUE: the submodule's feedthrough output net is ALSO read by an internal gate driving a second output, so the alias net has a load on both sides of the boundary.

module s (input a, output y1, output y2);
  assign y1 = a;
  INV_X1 g0 (.A(y1), .ZN(y2));
endmodule

module top (input i, input zi, output o1, output o2, output zo);
  s u0 (.a(i), .y1(o1), .y2(o2));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
