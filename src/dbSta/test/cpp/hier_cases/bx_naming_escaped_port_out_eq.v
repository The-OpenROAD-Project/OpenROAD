// TOP: top
// TECH: nangate45
// TARGETS: escaped_port, char_eq, depth_1
// CLUE: escaped top-level output port \z=o
module top (a, \z=o );
  input a;
  output \z=o ;
  INV_X1 u1 (.A(a), .ZN(\z=o ));
endmodule
