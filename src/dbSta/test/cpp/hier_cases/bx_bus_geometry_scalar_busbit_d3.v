// TOP: top
// TECH: nangate45
// TARGETS: scalar_port, bus_bit_landing, depth_3, assign_bit
// CLUE: Scalar port chain of depth 3 where at each level the scalar lands in a DIFFERENT internal bus at a different bit index via bit assigns.
module l3 (a, y);
  input a;
  output y;
  INV_X1 g (.A(a), .ZN(y));
endmodule
module l2 (a, y);
  input a;
  output y;
  wire [5:0] b2;
  wire [2:0] c2;
  assign b2[4] = a;
  l3 u (.a(b2[4]), .y(c2[1]));
  assign y = c2[1];
endmodule
module l1 (a, y);
  input a;
  output y;
  wire [3:0] b1;
  wire [7:0] c1;
  assign b1[2] = a;
  l2 u (.a(b1[2]), .y(c1[6]));
  assign y = c1[6];
endmodule
module top (x, z);
  input [7:0] x;
  output z;
  l1 u (.a(x[5]), .y(z));
endmodule
