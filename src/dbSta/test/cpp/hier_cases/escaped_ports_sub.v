// TOP: top
// TECH: nangate45
// TARGETS: escaped_name_ports, submodule_feedthrough
// CLUE: submodule whose ports are escaped names, connected by a port-to-port assign.

module esub (input \in! , output \out! );
  assign \out!  = \in! ;
endmodule

module top (input i, input zi, output o, output zo);
  esub u0 (.\in! (i), .\out! (o));
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
