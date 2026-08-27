// TOP: top
// TECH: nangate45
// TARGETS: hier, path_stamped_net_name, escaped_name_collision, dup_wire_decl
// CLUE: when name(Net) fails to strip, the flat path "u1/w" is emitted through
// staToVerilog as `\u1/w ` -- byte-identical to the emission of a user net whose
// sta name is "u1\/w" (from `\u1/w `), because staToVerilog DROPS the escape
// marker before '/'. Two electrically distinct nets in one module definition
// therefore get one name. Strip failure is forced with an escaped-slash driver
// name so every cone stays driven and self-check stays usable.
module top (a, b, y0, y1);
   input a;
   input b;
   output y0;
   output y1;
   m u1 (.i(a), .j(b), .o(y0), .p(y1));
endmodule

module m (i, j, o, p);
   input i;
   input j;
   output o;
   output p;
   wire w;
   wire \u1/w ;
   INV_X1 \g/1  (.A(i), .ZN(w));
   BUF_X1 g2 (.A(w), .Z(o));
   INV_X1 g3 (.A(j), .ZN(\u1/w ));
   BUF_X1 g4 (.A(\u1/w ), .Z(p));
endmodule
