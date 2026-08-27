// TOP: top
// TECH: nangate45
// TARGETS: bus_port_partial_bit_usage
// CLUE: dbReadVerilog.cc makeModITerms deliberately skips bus-aggregate
// dbModBTerms (isBusPort()) because STA pin iterators are bit-blasted and
// VerilogWriter would choke on an aggregate pin. A hierarchical bus port with
// only some bits connected exercises that skip.
module top (a, y);
   input [3:0] a;
   output [1:0] y;
   // Only bits 2 and 0 of the child's bus are used; 3 and 1 are left open.
   sub_sparse u_sparse (.d({a[3], a[2], a[1], a[0]}), .q0(y[0]), .q2(y[1]));
endmodule

module sub_sparse (d, q0, q2);
   input [3:0] d;
   output q0;
   output q2;
   BUF_X1 g0 (.A(d[0]), .Z(q0));
   BUF_X1 g2 (.A(d[2]), .Z(q2));
endmodule
