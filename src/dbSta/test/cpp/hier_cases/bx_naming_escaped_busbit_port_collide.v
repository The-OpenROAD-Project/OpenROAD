// TOP: top
// TECH: nangate45
// TARGETS: escaped_busbit_lookalike, port_collision, depth_1
// CLUE: scalar output port \q[2] coexists with real bus output [3:0] q -- distinct identifiers, emission hazard
module top (a, b, q, \q[2] );
  input a, b;
  output [3:0] q;
  output \q[2] ;
  BUF_X1 g0 (.A(a), .Z(q[0]));
  BUF_X1 g1 (.A(b), .Z(q[1]));
  AND2_X1 g2 (.A1(a), .A2(b), .ZN(q[2]));
  OR2_X1 g3 (.A1(a), .A2(b), .ZN(q[3]));
  XOR2_X1 g4 (.A(a), .B(b), .Z(\q[2] ));
endmodule
