// TOP: top
// TECH: nangate45
// TARGETS: escaped_name_bus_ports, bus_slice_assign, submodule
// CLUE: finding-2 bus-slice feedthrough where both submodule bus ports have ESCAPED names, so the alias route must survive name escaping too.

module sub (input [3:0] \a$in , output [1:0] \y.out );
  assign \y.out  = \a$in [3:2];
endmodule

module top (input [3:0] i, input zi, output [1:0] o, output zo);
  sub u0 (.\a$in (i), .\y.out (o));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
