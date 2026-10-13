// TOP: top
// TECH: nangate45
// TARGETS: output_readback, depth_2
// CLUE: The read-back of an output port happens two levels down, and the same
// CLUE: net is also an output of the enclosing module.

module top (a, y0, y1);
 input a;
 output y0, y1;
 mid u (.i(a), .o(y0), .z(y1));
endmodule

module mid (i, o, z);
 input i;
 output o, z;
 sub u (.i(i), .o(o), .z(z));
endmodule

module sub (i, o, z);
 input i;
 output o, z;
 NAND2_X1 g (.A1(i), .A2(i), .ZN(o));
 INV_X1 b (.A(o), .ZN(z));
endmodule
