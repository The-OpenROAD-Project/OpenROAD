// TOP: top
// TECH: nangate45
// TARGETS: hier, path_stamped_net_name, findPort_tail_fallback, escape_marker_loss
// CLUE: two branches in series. (1) the escaped-slash driver `\g/1 ` defeats
// name(Net)'s strip, so the net keeps the flat path "u1/z\/o". (2) writeWireDcls
// then asks findPort(cell, "u1/z\/o"); dbModule::findModBTerm's tail fallback cuts
// at the LAST '/' and finds the module's port o, so the declaration is suppressed.
// staToVerilog additionally drops the escape marker, so the emitted reference
// `\u1/z/o ` no longer denotes the same sta name it came from.
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
   wire \z/o ;
   INV_X1 \g/1  (.A(i), .ZN(\z/o ));
   BUF_X1 g2 (.A(\z/o ), .Z(q));
   BUF_X1 g3 (.A(j), .Z(o));
endmodule
