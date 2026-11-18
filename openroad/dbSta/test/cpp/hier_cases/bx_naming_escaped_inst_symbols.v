// TOP: top
// TECH: nangate45
// TARGETS: escaped_instance, char_symbols, depth_1
// CLUE: instance named \!@# ; pure punctuation instance name.
module top (input a, output z);
  INV_X1 \!@# (.A(a), .ZN(z));
endmodule
