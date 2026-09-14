// TOP: top
// TECH: nangate45
// TARGETS: escaped_slash_port_name, modbterm_tail_collision, order_control
// CLUE: same two ports as ..._tail_after but the escaped `\p/q ` is DECLARED
// FIRST, so dbModBTerm::create("p\/q") runs before port q exists and the
// tail fallback finds nothing. Order control: proves the trigger is creation
// order in makeModBTerms, not the '/' in the name.
module top (a, b, y0, y1);
   input a;
   input b;
   output y0;
   output y1;
   sub u (.q(a), .\p/q (b), .z0(y0), .z1(y1));
endmodule

module sub (\p/q , q, z0, z1);
   input \p/q ;
   input q;
   output z0;
   output z1;
   BUF_X1 g0 (.A(q), .Z(z0));
   INV_X1 g1 (.A(\p/q ), .ZN(z1));
endmodule
