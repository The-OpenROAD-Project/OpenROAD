// TOP: top
// TECH: nangate45
// TARGETS: escaped_instance, array_element_lookalike, depth_1
// CLUE: instance \g[0]  coexisting with plain instance g; unescaped it
// looks like an instance-array element of g.
module top (input a, output z, output y);
  INV_X1 g (.A(a), .ZN(y));
  INV_X1 \g[0] (.A(a), .ZN(z));
endmodule
