// TOP: top
// TECH: nangate45
// TARGETS: empty_module_no_ports, island_instance
// CLUE: a module with no ports and no contents instantiated in top with an
// empty connection list; degenerate hierarchy node — does the reader accept
// it and do the writers keep or drop it?

module nul;
endmodule

module top (input a, output z);
  BUF_X1 gb (.A(a), .Z(z));
  nul u_isl ();
endmodule
