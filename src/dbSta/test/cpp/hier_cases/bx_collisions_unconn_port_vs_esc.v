// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, synthesized_name_collision, unconnected_port
// CLUE: output port p of instance x is unconnected, so flattening must keep
// the internal port net under a synthesized name x/p -- which collides with
// the unrelated top escaped net \x/p .
module subq (input a, output p);
  INV_X1 g1 (.A(a), .ZN(p));
endmodule

module top (input in1, input in2, output o1, output o2);
  wire \x/p ;
  subq x (.a(in1), .p());
  INV_X1 g2 (.A(in2), .ZN(\x/p ));
  INV_X1 g3 (.A(\x/p ), .ZN(o1));
  BUF_X1 g4 (.A(in1), .Z(o2));
endmodule
