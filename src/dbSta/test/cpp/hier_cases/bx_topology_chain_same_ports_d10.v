// TOP: top
// TECH: nangate45
// TARGETS: chain_depth_10, identical_port_names, uniquification
// CLUE: 10-deep chain where EVERY level (including top) uses port names (i,o)
// and instance name u. Port-name collision across boundaries stresses
// hier net naming and flat name flattening.
module c9 (input i, output o);
  INV_X1 g (.A(i), .ZN(o));
endmodule

module c8 (input i, output o);
  c9 u (.i(i), .o(o));
endmodule

module c7 (input i, output o);
  c8 u (.i(i), .o(o));
endmodule

module c6 (input i, output o);
  c7 u (.i(i), .o(o));
endmodule

module c5 (input i, output o);
  c6 u (.i(i), .o(o));
endmodule

module c4 (input i, output o);
  c5 u (.i(i), .o(o));
endmodule

module c3 (input i, output o);
  c4 u (.i(i), .o(o));
endmodule

module c2 (input i, output o);
  c3 u (.i(i), .o(o));
endmodule

module c1 (input i, output o);
  c2 u (.i(i), .o(o));
endmodule

module top (input i, output o);
  c1 u (.i(i), .o(o));
endmodule
