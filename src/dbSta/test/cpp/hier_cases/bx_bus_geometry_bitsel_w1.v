// TOP: top
// TECH: nangate45
// TARGETS: bus_range_00, bit_select, assign_bit, depth_1
// CLUE: Bit-select w[0] of a [0:0] wire used as port connection, plus an assign to that single bit.
module sub (a, y);
  input a;
  output y;
  INV_X1 g0 (.A(a), .ZN(y));
endmodule
module top (x, z);
  input x;
  output z;
  wire [0:0] w;
  assign w[0] = x;
  sub s (.a(w[0]), .y(z));
endmodule
