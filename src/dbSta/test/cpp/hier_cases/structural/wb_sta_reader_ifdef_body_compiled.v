// TOP: top
// TECH: nangate45
// TARGETS: compiler_directive, ifdef, preprocessor
// CLUE: VerilogLex.ll:69 throws away any line starting with a backtick but never
// CLUE: implements the directive, so BOTH arms of an `ifdef/`else are parsed.  The
// CLUE: excluded INV_X1 g2 comes back (renamed by STA-1396) and multi-drives y.
module top (a, y);
  input a;
  output y;
  wire n;
  INV_X1 g1 (.A(a), .ZN(n));
`ifdef SPARE_INVERTER
  INV_X1 g2 (.A(n), .ZN(y));
`else
  BUF_X1 g2 (.A(n), .Z(y));
`endif
endmodule
