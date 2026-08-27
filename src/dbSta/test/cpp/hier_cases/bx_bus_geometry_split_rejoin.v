// TOP: top
// TECH: nangate45
// TARGETS: bus_split_two_ports, rejoin_lower_level, depth_2
// CLUE: Top splits in[3:0] across two child ports (lo/hi); mid re-joins them
// into one 4-bit port of a grandchild via {hi,lo}.
module leaf (p, q);
  input [3:0] p;
  output [3:0] q;
  INV_X1 g0 (.A(p[0]), .ZN(q[0]));
  BUF_X1 g1 (.A(p[1]), .Z(q[1]));
  NAND2_X1 g2 (.A1(p[2]), .A2(p[3]), .ZN(q[2]));
  XOR2_X1 g3 (.A(p[3]), .B(p[0]), .Z(q[3]));
endmodule
module mid (lo, hi, z);
  input [1:0] lo;
  input [1:0] hi;
  output [3:0] z;
  leaf u0 (.p({hi, lo}), .q(z));
endmodule
module top (in, out);
  input [3:0] in;
  output [3:0] out;
  mid u0 (.lo(in[1:0]), .hi(in[3:2]), .z(out));
endmodule
