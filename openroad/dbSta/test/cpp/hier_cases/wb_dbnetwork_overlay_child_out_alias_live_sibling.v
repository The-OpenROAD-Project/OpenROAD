// TOP: top
// TECH: nangate45
// TARGETS: hier, output_port_alias, partial_damage, internal_reader
// CLUE: variant of the output-port alias where the surviving net w ALSO feeds a
// load inside the module. The internal cone (q) must stay intact while the aliased
// port o is the only casualty, which separates "the modnet for the port is lost"
// from "the net itself is lost" -- and gives LEC a live reference cone so a
// coverage/counterexample split is visible.
module top (a, y0, y1);
   input a;
   output y0;
   output y1;
   wire t;
   m u1 (.i(a), .o(t), .q(y1));
   BUF_X1 gt (.A(t), .Z(y0));
endmodule

module m (i, o, q);
   input i;
   output o;
   output q;
   wire w;
   INV_X1 g1 (.A(i), .ZN(w));
   BUF_X1 g2 (.A(w), .Z(q));
   assign o = w;
endmodule
