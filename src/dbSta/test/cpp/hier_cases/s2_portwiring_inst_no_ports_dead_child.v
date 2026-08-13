// TARGETS: empty_port_list, sub_input_unconnected, sub_output_dangling, depth_1
// CLUE: A hierarchical instance with an EMPTY connection list: every port of
// CLUE: the child is unconnected at once, so the writer has to emit a module
// CLUE: instantiation with no ports at all beside a live top-level cone.

module top (a, y);
 input a;
 output y;
 dead u ();
 INV_X1 g (.A(a), .ZN(y));
endmodule

module dead (i, o);
 input i;
 output o;
 BUF_X1 b (.A(i), .Z(o));
endmodule
