// TOP: top
// TECH: nangate45
// TARGETS: top_input_unused, dollar_name
// CLUE: dangling top input with a $ in its plain identifier (nc$1). Dangling
// object plus a legal-but-unusual character: renaming would be visible.
module top (x, nc$1, y);
  input x;
  input nc$1;
  output y;
  INV_X1 u1 (.A(x), .ZN(y));
endmodule
