// TOP: top
// TECH: nangate45
// TARGETS: module_name_prefix_trap, m_m1_m2_m3_exist, m_four_times
// CLUE: modules m, m_1, m_2, m_3 all exist as distinct types AND m is
// instantiated four times; naive uniquification of m produces m_1..m_3 which
// all collide with real modules.

module m (input i, output o);
  NAND2_X1 g (.A1(i), .A2(i), .ZN(o));
endmodule

module m_1 (input i, output o);
  INV_X1 g (.A(i), .ZN(o));
endmodule

module m_2 (input i, output o);
  BUF_X1 g (.A(i), .Z(o));
endmodule

module m_3 (input i, output o);
  NOR2_X1 g (.A1(i), .A2(i), .ZN(o));
endmodule

module top (input [3:0] a, input [2:0] b, output [3:0] x, output [2:0] y);
  m u0 (.i(a[0]), .o(x[0]));
  m u1 (.i(a[1]), .o(x[1]));
  m u2 (.i(a[2]), .o(x[2]));
  m u3 (.i(a[3]), .o(x[3]));
  m_1 v1 (.i(b[0]), .o(y[0]));
  m_2 v2 (.i(b[1]), .o(y[1]));
  m_3 v3 (.i(b[2]), .o(y[2]));
endmodule
