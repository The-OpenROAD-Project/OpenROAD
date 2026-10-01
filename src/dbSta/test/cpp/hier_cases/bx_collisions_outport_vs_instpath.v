// TOP: top
// TECH: nangate45
// TARGETS: escaped_port, flat_path_collision, cross_kind_collision
// CLUE: top OUTPUT PORT \x/y vs INSTANCE y inside hierarchy x; the flat
// output declares an output net x/y and an instance x/y in one module scope
// (one namespace per LRM) -- illegal even though connectivity is intact.
module subo (input a, output z);
  INV_X1 y (.A(a), .ZN(z));
endmodule

module top (input i1, input i2, output o1, output \x/y );
  subo x (.a(i1), .z(o1));
  INV_X1 g1 (.A(i2), .ZN(\x/y ));
endmodule
