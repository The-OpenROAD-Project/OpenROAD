// TOP: top
// TECH: nangate45
// TARGETS: escaped_lexes_plain, instance, depth_1
// CLUE: instance named \u1  (same identifier as plain u1); writer should
// treat it as u1 or re-escape consistently.
module top (input a, output z);
  INV_X1 \u1 (.A(a), .ZN(z));
endmodule
