// TOP: top
// TECH: nangate45
// TARGETS: escaped_slash_port_name, modbterm_tail_collision, depth_2
// CLUE: depth-2 generalization of the tail collision: the mid-level module
// carries both `q` and `\p/q ` and forwards them to a leaf module. Tests that
// the collision is per-module (mid corrupted, leaf clean) and that the guard
// added at dbReadVerilog.cc:668 (full_name == getHierarchicalName) does not
// protect a module's OWN escaped port.
module top (a, b, y0, y1);
   input a;
   input b;
   output y0;
   output y1;
   mid u_mid (.q(a), .\p/q (b), .z0(y0), .z1(y1));
endmodule

module mid (q, \p/q , z0, z1);
   input q;
   input \p/q ;
   output z0;
   output z1;
   leaf u_leaf (.i0(q), .i1(\p/q ), .o0(z0), .o1(z1));
endmodule

module leaf (i0, i1, o0, o1);
   input i0;
   input i1;
   output o0;
   output o1;
   BUF_X1 g0 (.A(i0), .Z(o0));
   INV_X1 g1 (.A(i1), .ZN(o1));
endmodule
