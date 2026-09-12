// TOP: top
// TECH: nangate45
// TARGETS: escaped_slash_port_name, control
// CLUE: control for the tail-collision family: an escaped port `\p/q ` whose
// trailing segment "q" matches NO other port of the module. The
// findModBTerm/staToDb '/' stripping still fires but resolves to nothing, so
// the module should round-trip intact.
module top (a, b, y0, y1);
   input a;
   input b;
   output y0;
   output y1;
   sub u (.r(a), .\p/q (b), .z0(y0), .z1(y1));
endmodule

module sub (r, \p/q , z0, z1);
   input r;
   input \p/q ;
   output z0;
   output z1;
   BUF_X1 g0 (.A(r), .Z(z0));
   INV_X1 g1 (.A(\p/q ), .ZN(z1));
endmodule
