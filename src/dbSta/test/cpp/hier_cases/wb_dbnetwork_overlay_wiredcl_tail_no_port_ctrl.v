// TOP: top
// TECH: nangate45
// TARGETS: control, hier, findPort_tail_fallback
// CLUE: Control for ..._wiredcl_tail_eq_port: same escaped-slash net, but its
// tail "z" is NOT a port of the module, so dbModule::findModBTerm's fallback
// misses and the wire declaration must be emitted. Isolates the tail-name
// collision as the trigger rather than the escaped slash itself.
module top (a, b, y0, y1);
   input a;
   input b;
   output y0;
   output y1;
   m u1 (.i(a), .j(b), .o(y0), .q(y1));
endmodule

module m (i, j, o, q);
   input i;
   input j;
   output o;
   output q;
   wire \x/z ;
   INV_X1 g1 (.A(i), .ZN(\x/z ));
   BUF_X1 g2 (.A(\x/z ), .Z(q));
   BUF_X1 g3 (.A(j), .Z(o));
endmodule
