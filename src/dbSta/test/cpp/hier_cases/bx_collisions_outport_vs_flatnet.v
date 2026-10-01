// TOP: top
// TECH: nangate45
// TARGETS: escaped_port, flat_net_collision, output_port
// CLUE: top OUTPUT PORT named \x/y collides with flattened internal net y of
// hierarchy x; a merge would double-drive the output port.
module subn4 (input a, output z);
  wire y;
  INV_X1 g1 (.A(a), .ZN(y));
  INV_X1 g2 (.A(y), .ZN(z));
endmodule

module top (input in1, input in2, output o1, output \x/y );
  subn4 x (.a(in1), .z(o1));
  INV_X1 g3 (.A(in2), .ZN(\x/y ));
endmodule
