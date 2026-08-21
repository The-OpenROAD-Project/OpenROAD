// TOP: top
// TECH: nangate45
// TARGETS: sub_bus_input_unconnected, nc_filler_vs_user_net, depth_2
// CLUE: the _NC filler must be created in the PARENT of the open instance. Here
// that parent is mid (depth 1) which already owns a live net named _NC1.
module leaf (input a, input [1:0] db, output y);
  INV_X1 u1 (.A(a), .ZN(y));
endmodule
module mid (input a, output y);
  wire _NC1;
  BUF_X1 b (.A(a), .Z(_NC1));
  leaf u_l (.a(_NC1), .y(y));
endmodule
module top (input x, output y);
  mid u_m (.a(x), .y(y));
endmodule
