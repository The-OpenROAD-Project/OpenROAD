// TOP: top
// TECH: nangate45
// TARGETS: control, hier, escaped_slash_net_name, name_strip_ok
// CLUE: Control isolating the pivot of ..._netname_drvr_escslash: here the NET
// carries the escaped slash and the DRIVER is plainly named. header_to_remove is
// then the honest "u1", find() hits at position 0 and the erase is correct, so
// `\g/x ` must survive as `\g/x `. Proves it is the driver instance name -- not
// the presence of an escaped slash anywhere -- that defeats the strip.
module top (a, y);
   input a;
   output y;
   m u1 (.i(a), .o(y));
endmodule

module m (i, o);
   input i;
   output o;
   wire \g/x ;
   INV_X1 g1 (.A(i), .ZN(\g/x ));
   BUF_X1 g2 (.A(\g/x ), .Z(o));
endmodule
