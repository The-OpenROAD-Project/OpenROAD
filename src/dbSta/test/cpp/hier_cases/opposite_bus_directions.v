// TOP: top
// TECH: nangate45
// TARGETS: same_named_buses_opposite_direction
// CLUE: dbReadVerilog.cc recordBusPortsOrder keys its property on
// "bus_msb_first <port> <cell>", so two modules declaring a same-named bus in
// opposite directions must not share one annotation.
module top (a, y0, y1);
   input [3:0] a;
   output [3:0] y0;
   output [3:0] y1;
   mod_up   u_up   (.d(a), .q(y0));
   mod_down u_down (.d(a), .q(y1));
endmodule

module mod_up (d, q);
   input [0:3] d;
   output [0:3] q;
   BUF_X1 g0 (.A(d[0]), .Z(q[0]));
   BUF_X1 g1 (.A(d[1]), .Z(q[1]));
   BUF_X1 g2 (.A(d[2]), .Z(q[2]));
   BUF_X1 g3 (.A(d[3]), .Z(q[3]));
endmodule

module mod_down (d, q);
   input [3:0] d;
   output [3:0] q;
   INV_X1 g0 (.A(d[0]), .ZN(q[0]));
   INV_X1 g1 (.A(d[1]), .ZN(q[1]));
   INV_X1 g2 (.A(d[2]), .ZN(q[2]));
   INV_X1 g3 (.A(d[3]), .ZN(q[3]));
endmodule
