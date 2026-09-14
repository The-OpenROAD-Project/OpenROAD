// TOP: top
// TECH: nangate45
// TARGETS: hier, output_port_alias, flat_name_captured_in_child_scope, short
// CLUE: sharpened ..._child_out_alias_internal. When the aliased driver falls back
// to its flat dbNet, the emitted name is the TOP-scope net name ("t"), which is
// re-interpreted in the CHILD module's namespace. Here module m owns its own net
// named t (driven by g2), so the fallback name is captured: g1 and g2 should end
// up driving one net inside m, shorting two independent cones.
module top (a, b, y0, y1);
   input a;
   input b;
   output y0;
   output y1;
   wire t;
   m u1 (.i(a), .j(b), .o(t), .q(y1));
   BUF_X1 gt (.A(t), .Z(y0));
endmodule

module m (i, j, o, q);
   input i;
   input j;
   output o;
   output q;
   wire w;
   wire t;
   INV_X1 g1 (.A(i), .ZN(w));
   assign o = w;
   NAND2_X1 g2 (.A1(j), .A2(i), .ZN(t));
   BUF_X1 g3 (.A(t), .Z(q));
endmodule
