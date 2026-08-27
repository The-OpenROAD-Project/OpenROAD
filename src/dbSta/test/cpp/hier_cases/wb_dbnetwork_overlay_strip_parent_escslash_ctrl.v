// TOP: top
// TECH: nangate45
// TARGETS: control, hier, stripParentPrefix, escaped_slash_modinst
// CLUE: Control for ..._strip_parent_trailing_bslash. A module instance named
// `\u1/x ` gives sta "u1\/x", so the flat leaf name "u1\/x/g1" has 'x' (not '\')
// before the real divider: stripParentPrefix's guard works and the leaf must come
// back as plain g1. Isolates "backslash immediately left of the divider" as the
// trigger, rather than "escaped instance name".
module top (a, y);
   input a;
   output y;
   m \u1/x  (.i(a), .o(y));
endmodule

module m (i, o);
   input i;
   output o;
   wire w;
   INV_X1 g1 (.A(i), .ZN(w));
   BUF_X1 g2 (.A(w), .Z(o));
endmodule
