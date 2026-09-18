// TOP: top
// TECH: nangate45
// TARGETS: dangling_net, escaped_net_name
// CLUE: escaped wire \dead*net is driven by INV g2 but never read. Does the escaped
// name survive with correct escaping?
module top (input in1, output out1);
  wire \dead*net ;
  INV_X1 g1 (.A(in1), .ZN(out1));
  INV_X1 g2 (.A(in1), .ZN(\dead*net ));
endmodule
