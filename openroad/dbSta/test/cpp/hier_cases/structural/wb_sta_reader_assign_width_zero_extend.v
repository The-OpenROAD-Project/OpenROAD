// TOP: top
// TECH: nangate45
// TARGETS: assign_width, zero_extend, drop
// CLUE: mergeAssignNet warns STA-0203 and drops the WHOLE assign when the sizes differ
// CLUE: (VerilogReader.cc:1863), instead of the LRM's zero-extend, so y[1] loses its
// CLUE: constant 0 and y[0] loses its driver.  Distinct code site from STA-0200/0202.
module top (a, y);
  input a;
  output [1:0] y;
  wire n;
  INV_X1 g (.A(a), .ZN(n));
  assign y = n;
endmodule
