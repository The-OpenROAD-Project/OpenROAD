// TARGETS: two_synthesized_net_paths_collide, unconnected_port, escaped_inst, depth_3
// CLUE: both colliding names come from the SAME generator -- the net kept for an
// unconnected submodule output port -- reached by two different paths.  Instance
// y of x leaves p open, giving x/y/p; instance \x/y leaves p open, giving
// x\/y/p.  Both print \x/y/p .  The two submodules are distinct cells with
// distinct gate names so the ONLY colliding pair is the two dangling nets;
// nothing else under the shared prefix can be blamed.
module dsuba (input a, output p, output z);
  INV_X1 ga1 (.A(a), .ZN(p));
  BUF_X1 ga2 (.A(a), .Z(z));
endmodule

module dsubb (input a, output p, output z);
  INV_X1 gb1 (.A(a), .ZN(p));
  BUF_X1 gb2 (.A(a), .Z(z));
endmodule

module dmid (input a, output z);
  dsuba y (.a(a), .p(), .z(z));
endmodule

module top (input i1, input i2, output o1, output o2);
  dmid x (.a(i1), .z(o1));
  dsubb \x/y  (.a(i2), .p(), .z(o2));
endmodule
