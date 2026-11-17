// TOP: top
// TECH: nangate45
// TARGETS: undriven_bus_net_in_module, name_leak, bus_shape
// CLUE: undriven BUS net inside sub feeds two dead gates. Checks whether the hier
// name leak also mangles bus nets and whether the bus shape survives.
module sub (input a, output y);
  wire [1:0] und;
  wire d0;
  wire d1;
  INV_X1 g1 (.A(a), .ZN(y));
  INV_X1 g2 (.A(und[0]), .ZN(d0));
  INV_X1 g3 (.A(und[1]), .ZN(d1));
endmodule
module top (input in1, output out1);
  sub u1 (.a(in1), .y(out1));
endmodule
