// TOP: top
// TECH: nangate45
// TARGETS: control, hier, escaped_bracket_driver_name, name_strip_ok
// CLUE: control pinning the trigger character. verilogToSta escapes '[' and ']'
// exactly like '/', but only '/' is a hierarchy divider, so a driver named
// `\g[0] ` (sta "g\[0\]") leaves find_last_of('/') pointing at the real divider:
// header_to_remove is the honest "u1" and the local net must survive as `w`.
module top (a, y);
   input a;
   output y;
   m u1 (.i(a), .o(y));
endmodule

module m (i, o);
   input i;
   output o;
   wire w;
   INV_X1 \g[0]  (.A(i), .ZN(w));
   BUF_X1 g2 (.A(w), .Z(o));
endmodule
