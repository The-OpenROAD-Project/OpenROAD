// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, bus_net, char_plus, depth_1
// CLUE: BUS net named \v+w  (wire [1:0]) with bit-selects \v+w [i];
// escaped identifier as a vector base name.
module top (input a, input b, output z);
  wire [1:0] \v+w ;
  BUF_X1 g1 (.A(a), .Z(\v+w [0]));
  BUF_X1 g2 (.A(b), .Z(\v+w [1]));
  XOR2_X1 g3 (.A(\v+w [0]), .B(\v+w [1]), .Z(z));
endmodule
