// TARGETS: sub_input_unconnected, sub_output_dangling, both_unconnected, depth_2
// CLUE: ONE instance has an explicitly empty INPUT port and an explicitly empty
// CLUE: OUTPUT port at the same time, at depth 2, while its other two ports
// CLUE: stay live. Both empties are named (.i1(), .o1()), so the parser keeps
// CLUE: the port in the list and the binder must drop just that one wire.

module top (a, b, y, z);
 input a;
 input b;
 output y;
 output z;
 mid u (.p(a), .q(b), .r(y), .s(z));
endmodule

module mid (p, q, r, s);
 input p;
 input q;
 output r;
 output s;
 sub u (.i0(p), .i1(), .o0(r), .o1());
 BUF_X1 g (.A(q), .Z(s));
endmodule

module sub (i0, i1, o0, o1);
 input i0;
 input i1;
 output o0;
 output o1;
 INV_X1 g0 (.A(i0), .ZN(o0));
 BUF_X1 g1 (.A(i1), .Z(o1));
endmodule
