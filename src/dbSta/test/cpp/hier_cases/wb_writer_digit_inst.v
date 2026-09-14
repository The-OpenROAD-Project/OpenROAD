// TOP: top
// TECH: nangate45
// TARGETS: needs_escape, leading_digit, instance_name
// CLUE: instanceVerilogName -> staToVerilog has the same alnum-only test, so an escaped
// CLUE: instance name that starts with a digit comes back unescaped: "INV_X1 1g (...)".
module top (a, y);
  input a;
  output y;
  wire n;
  INV_X1 \1g  (.A(a), .ZN(n));
  BUF_X1 g2 (.A(n), .Z(y));
endmodule
