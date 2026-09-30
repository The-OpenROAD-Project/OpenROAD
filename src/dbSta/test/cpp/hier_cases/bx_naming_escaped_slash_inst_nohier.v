// TOP: top
// TECH: nangate45
// TARGETS: escaped_instance, char_slash, no_collision, depth_1
// CLUE: instance named \p/q  with NO module or instance named p anywhere;
// baseline for the slash-collision family.
module top (input a, output z);
  INV_X1 \p/q (.A(a), .ZN(z));
endmodule
