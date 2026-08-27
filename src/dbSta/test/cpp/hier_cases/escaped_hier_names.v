// TOP: top
// TECH: nangate45
// TARGETS: escaped_identifiers_with_slash_and_brackets
// CLUE: dbReadVerilog.cc stripParentPrefix is escaped-slash aware -- a `\/`
// inside an escaped identifier is NOT a hierarchy separator. Flat
// write_verilog builds names by joining the path with '/', so an escaped
// identifier that already contains '/', '[' or ']' can collide with a
// synthesized flat name. Classes (H) #8772, #6405.
module top (a, b, y0, y1);
   input a, b;
   output y0, y1;
   wire \net/with/slash ;
   wire \net[3] ;
   \mod/slash  \u/inst  (.i(a), .o(\net/with/slash ));
   BUF_X1 \g[0]  (.A(\net/with/slash ), .Z(y0));
   \mod/slash  u_plain (.i(b), .o(\net[3] ));
   BUF_X1 g1 (.A(\net[3] ), .Z(y1));
endmodule

module \mod/slash  (i, o);
   input i;
   output o;
   INV_X1 \inv/0  (.A(i), .ZN(o));
endmodule
