// TOP: top
// TECH: nangate45
// TARGETS: uniquification_scheme_collision, module_named_m_u1_exists
// CLUE: hier link uniquifies the 2nd instance of m (named u1) into module
// name m_u1 — but a REAL distinct module m_u1 already exists and is
// instantiated. If uniquification does not check existing names, two
// different modules named m_u1 result.

module m (input i, output o);
  NAND2_X1 g (.A1(i), .A2(i), .ZN(o));
endmodule

module m_u1 (input i, output o);
  INV_X1 g (.A(i), .ZN(o));
endmodule

module top (input a, input b, input c, output x, output y, output z);
  m u0 (.i(a), .o(x));
  m u1 (.i(b), .o(y));
  m_u1 w1 (.i(c), .o(z));
endmodule
