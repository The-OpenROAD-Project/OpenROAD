// TOP: top
// TECH: nangate45
// TARGETS: submodule_bus_order, msb_first_order
// CLUE: dbReadVerilog.cc recordBusPortsOrder iterates only the TOP cell's
// ports, so a submodule's bus direction is never recorded as a
// bus_msb_first property for VerilogWriter to consult.
module top (a, y);
   input [3:0] a;
   output [3:0] y;
   sub_desc u_desc (.si(a), .so(y));
endmodule

// Ascending (lsb-first) bus ports, the opposite order from top's.
module sub_desc (si, so);
   input [0:3] si;
   output [0:3] so;
   BUF_X1 g0 (.A(si[0]), .Z(so[0]));
   BUF_X1 g1 (.A(si[1]), .Z(so[1]));
   BUF_X1 g2 (.A(si[2]), .Z(so[2]));
   BUF_X1 g3 (.A(si[3]), .Z(so[3]));
endmodule
