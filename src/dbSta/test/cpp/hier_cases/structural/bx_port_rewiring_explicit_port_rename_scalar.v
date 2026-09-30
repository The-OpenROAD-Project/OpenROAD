// TOP: top
// TECH: nangate45
// TARGETS: explicit_header_port, name_decoupled, depth_1
// CLUE: External port names pi/po bind to internal nets x/z via
// CLUE: Verilog-2001 explicit port syntax. A reader that keys the port by the
// CLUE: internal net name (or vice versa) mis-binds the boundary.

module top (a, y);
 input a;
 output y;
 sub u (.pi(a), .po(y));
endmodule

module sub (.pi(x), .po(z));
 input x;
 output z;
 INV_X1 g (.A(x), .ZN(z));
endmodule
