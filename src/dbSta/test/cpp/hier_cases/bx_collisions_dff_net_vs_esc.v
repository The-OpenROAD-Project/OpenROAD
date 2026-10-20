// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, flat_net_collision, sequential
// CLUE: sequential variant of the net collision: the submodule net that
// collides is a flop output (q inside x), so a merge shorts a DFF Q pin with
// an unrelated combinational driver.
module subd (input a, input ck, output z);
  wire q;
  DFF_X1 f1 (.D(a), .CK(ck), .Q(q));
  INV_X1 g1 (.A(q), .ZN(z));
endmodule

module top (input in1, input in2, input clk, output o1, output o2);
  wire \x/q ;
  subd x (.a(in1), .ck(clk), .z(o1));
  INV_X1 g3 (.A(in2), .ZN(\x/q ));
  INV_X1 g4 (.A(\x/q ), .ZN(o2));
endmodule
