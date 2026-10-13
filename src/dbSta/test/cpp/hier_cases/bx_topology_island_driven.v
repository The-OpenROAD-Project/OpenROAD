// TOP: top
// TECH: nangate45
// TARGETS: island_submodule, unobservable_logic, dangling_output
// CLUE: submodule isl is driven by a top input but its output net goes
// nowhere observable; LEC cannot see it — observe structurally whether the
// island instance and its gate survive both writers.

module isl (input i, output o);
  INV_X1 g (.A(i), .ZN(o));
endmodule

module top (input a, output z);
  wire nc;
  BUF_X1 gb (.A(a), .Z(z));
  isl u_isl (.i(a), .o(nc));
endmodule
