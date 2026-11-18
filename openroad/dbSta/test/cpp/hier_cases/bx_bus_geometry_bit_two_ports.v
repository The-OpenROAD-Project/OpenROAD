// TOP: top
// TECH: nangate45
// TARGETS: same_bit_two_ports, one_instance
// CLUE: Same bus bit x[2] wired to two different input ports of ONE child
// instance; net-to-port map is many-to-one.
module sub (a, b, z1, z2);
  input a;
  input b;
  output z1;
  output z2;
  INV_X1 g0 (.A(a), .ZN(z1));
  BUF_X1 g1 (.A(b), .Z(z2));
endmodule
module top (x, o1, o2);
  input [3:0] x;
  output o1;
  output o2;
  sub u0 (.a(x[2]), .b(x[2]), .z1(o1), .z2(o2));
endmodule
