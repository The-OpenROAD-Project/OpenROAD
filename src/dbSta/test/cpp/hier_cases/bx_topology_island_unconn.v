// TOP: top
// TECH: nangate45
// TARGETS: island_submodule, all_ports_explicitly_unconnected
// CLUE: instance of a gated submodule with BOTH ports explicitly unconnected
// (.i(), .o()); inside, the gate input floats. Observe whether the instance
// survives and how unconnected ports are re-emitted.

module isl2 (input i, output o);
  INV_X1 g (.A(i), .ZN(o));
endmodule

module top (input a, output z);
  BUF_X1 gb (.A(a), .Z(z));
  isl2 u_isl (.i(), .o());
endmodule
