// TOP: top
// TECH: nangate45
// TARGETS: named_conn, empty_named, dangling_in
// CLUE: Explicit empty named connection .di() on an unused child input port.

module top (a, y);
 input a;
 output y;
 sub u (.i(a), .di(), .o(y));
endmodule

module sub (i, di, o);
 input i, di;
 output o;
 BUF_X1 b (.A(i), .Z(o));
endmodule
