// TOP: top
// TECH: nangate45
// TARGETS: needs_escape, leading_digit, port_name
// CLUE: portVerilogName -> staToVerilog2, same alnum-only test, so a digit-leading
// CLUE: escaped port name is emitted bare in both the header and the direction dcl.
module top (\1p , y);
  input \1p ;
  output y;
  INV_X1 g (.A(\1p ), .ZN(y));
endmodule
