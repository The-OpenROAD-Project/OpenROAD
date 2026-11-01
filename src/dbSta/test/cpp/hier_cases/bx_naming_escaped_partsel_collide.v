// TOP: top
// TECH: nangate45
// TARGETS: escaped_partselect_lookalike, collision, depth_1
// CLUE: scalar net \v[1:0] coexists with real bus wire [1:0] v used bitwise
module top (a, b, y, z);
  input a, b;
  output y, z;
  wire [1:0] v;
  wire \v[1:0] ;
  BUF_X1 g0 (.A(a), .Z(v[0]));
  BUF_X1 g1 (.A(b), .Z(v[1]));
  XOR2_X1 g2 (.A(v[0]), .B(v[1]), .Z(y));
  AND2_X1 g3 (.A1(a), .A2(b), .ZN(\v[1:0] ));
  INV_X1 g4 (.A(\v[1:0] ), .ZN(z));
endmodule
