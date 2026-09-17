// TOP: top
// TECH: nangate45
// TARGETS: escaped_slash_port_name, modbterm_tail_collision, hier
// CLUE: verilogToSta stores escaped `\p/q ` as the sta name "p\/q", which
// contains a RAW '/'. dbModule::findModBTerm (dbModule.cpp:566-573) falls back
// to the substring after the last '/', so dbModBTerm::create("p\/q") returns
// the ALREADY EXISTING port `q` instead of making a new one. Port q is declared
// first here, so the child module loses its \p/q port entirely.
module top (a, b, y0, y1);
   input a;
   input b;
   output y0;
   output y1;
   sub u (.q(a), .\p/q (b), .z0(y0), .z1(y1));
endmodule

module sub (q, \p/q , z0, z1);
   input q;
   input \p/q ;
   output z0;
   output z1;
   BUF_X1 g0 (.A(q), .Z(z0));
   INV_X1 g1 (.A(\p/q ), .ZN(z1));
endmodule
