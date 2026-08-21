// TOP: top
// TECH: nangate45
// TARGETS: island_submodule, self_contained_constant_island
// CLUE: submodule with NO inputs — internal LOGIC1 drives an inverter whose
// output leaves the module into a dangling net; a fully self-contained
// island; observe survival structurally.

module iconst (output o);
  wire c;
  LOGIC1_X1 t (.Z(c));
  INV_X1 g (.A(c), .ZN(o));
endmodule

module top (input a, output z);
  wire nc;
  BUF_X1 gb (.A(a), .Z(z));
  iconst u_isl (.o(nc));
endmodule
