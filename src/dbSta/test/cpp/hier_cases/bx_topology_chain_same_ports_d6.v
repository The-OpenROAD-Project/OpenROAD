// TOP: top
// TECH: nangate45
// TARGETS: chain_depth_6, identical_port_names, identical_instance_name
// CLUE: 6-deep chain where every level (including top) uses port names (i,o)
// and instance name u; shallower bracket of the depth-10 same-ports chain.

module s5 (input i, output o);
  INV_X1 g (.A(i), .ZN(o));
endmodule

module s4 (input i, output o);
  s5 u (.i(i), .o(o));
endmodule

module s3 (input i, output o);
  s4 u (.i(i), .o(o));
endmodule

module s2 (input i, output o);
  s3 u (.i(i), .o(o));
endmodule

module s1 (input i, output o);
  s2 u (.i(i), .o(o));
endmodule

module top (input i, output o);
  s1 u (.i(i), .o(o));
endmodule
