// TOP: top
// TECH: nangate45
// TARGETS: escaped_instance, cellname_shadow, depth_1
// CLUE: instance escaped-named \INV_X1  of cell INV_X1; normalizes to a
// plain instance name equal to its own cell type (legal, separate
// namespaces) -- probes name-uniquification logic.
module top (input a, output z);
  INV_X1 \INV_X1 (.A(a), .ZN(z));
endmodule
