// TOP: top
// TECH: nangate45
// TARGETS: uniquification_scheme_collision, numeric_fallback_trap
// CLUE: modules m, m_u1 AND m_u1_1 all exist while m is instantiated twice
// with the second instance named u1; both the primary uniquified name (m_u1)
// and its first numeric fallback (m_u1_1) are taken by real modules.

module m (input i, output o);
  NAND2_X1 g (.A1(i), .A2(i), .ZN(o));
endmodule

module m_u1 (input i, output o);
  INV_X1 g (.A(i), .ZN(o));
endmodule

module m_u1_1 (input i, output o);
  BUF_X1 g (.A(i), .Z(o));
endmodule

module top (input a, input b, input c, input d,
            output w, output x, output y, output z);
  m u0 (.i(a), .o(w));
  m u1 (.i(b), .o(x));
  m_u1 w1 (.i(c), .o(y));
  m_u1_1 w2 (.i(d), .o(z));
endmodule
