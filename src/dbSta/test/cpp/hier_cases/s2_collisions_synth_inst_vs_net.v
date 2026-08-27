// TARGETS: synthesized_name_cross_namespace, net_vs_inst, unconnected_port, depth_3
// CLUE: a synthesized NET name and a synthesized INSTANCE name collide, with no
// user object owning either.  Leaf instance z inside y inside x flattens to the
// instance path x/y/z; the open output port z of instance \x/y keeps its net as
// x\/y/z.  Verilog puts nets and instances in ONE module namespace, so the
// emitted module declares `wire \x/y/z ;` and instantiates `INV_X1 \x/y/z `.
module iB (input a, output o);
  INV_X1 z (.A(a), .ZN(o));
endmodule

module iA (input a, output o);
  iB y (.a(a), .o(o));
endmodule

module iC (input a, output z, output o);
  INV_X1 g1 (.A(a), .ZN(z));
  BUF_X1 g2 (.A(a), .Z(o));
endmodule

module top (input i1, input i2, output o1, output o2);
  iA x (.a(i1), .o(o1));
  iC \x/y  (.a(i2), .z(), .o(o2));
endmodule
