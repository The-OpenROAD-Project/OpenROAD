// TOP: top
// TECH: nangate45
// TARGETS: escaped_instance, keyword_assign, depth_2
// CLUE: instance named \assign  INSIDE a submodule; flat prefixing makes
// u1/assign (slash forces escaping, masking the keyword bug) while the
// hier writer re-emits the bare keyword -- brackets inst_kw_assign.
module suba (input a, output z);
  INV_X1 \assign (.A(a), .ZN(z));
endmodule
module top (input a, output z);
  suba u1 (.a(a), .z(z));
endmodule
