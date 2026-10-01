// TOP: top
// TECH: nangate45
// TARGETS: attribute, dont_touch, control
// CLUE: control for the string form: (* dont_touch = 1 *) is the only spelling
// CLUE: dbReadVerilog.cc:580 can parse.  Does the attribute survive write_verilog?
module top (a, y);
  input a;
  output y;
  wire n;
  (* dont_touch = 1 *) INV_X1 g1 (.A(a), .ZN(n));
  BUF_X1 g2 (.A(n), .Z(y));
endmodule
