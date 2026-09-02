// TOP: top
// TECH: nangate45
// TARGETS: alias_net_also_port_connected
// CLUE: the intermediate net of a top-level assign chain is ALSO the port connection of a submodule instance, so collapsing the alias must not orphan the pin.

module g (input a, output y);
  INV_X1 g0 (.A(a), .ZN(y));
endmodule

module top (input i, input zi, output o1, output o2, output zo);
  wire t;
  assign t = i;
  assign o1 = t;
  g u0 (.a(t), .y(o2));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
