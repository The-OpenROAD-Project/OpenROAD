// TOP: top
// TECH: nangate45
// TARGETS: output_readback, depth_1
// CLUE: A child reads its own output port net as an input to further logic
// CLUE: inside the same child.

module top (a, y0, y1);
 input a;
 output y0, y1;
 sub u (.i(a), .o(y0), .z(y1));
endmodule

module sub (i, o, z);
 input i;
 output o, z;
 INV_X1 g (.A(i), .ZN(o));
 BUF_X1 b (.A(o), .Z(z));
endmodule
