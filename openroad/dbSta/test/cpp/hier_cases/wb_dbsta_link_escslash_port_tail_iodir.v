// TOP: top
// TECH: nangate45
// TARGETS: escaped_slash_port_name, modbterm_tail_collision, io_direction
// CLUE: same collision as ..._tail_after but the escaped port is an OUTPUT
// whose tail matches an INPUT port. dbModBTerm::create returns the existing
// input modbterm and then setIoType(OUTPUT) rewrites the direction of the
// wrong port; Verilog2db::staToDb (dbReadVerilog.cc:648) additionally strips
// the pin name at '/' so the input's modITerm is rebound to the output net.
module top (a, y0, y1);
   input a;
   output y0;
   output y1;
   sub u (.q(a), .\p/q (y0), .z(y1));
endmodule

module sub (q, \p/q , z);
   input q;
   output \p/q ;
   output z;
   INV_X1 g0 (.A(q), .ZN(\p/q ));
   BUF_X1 g1 (.A(q), .Z(z));
endmodule
