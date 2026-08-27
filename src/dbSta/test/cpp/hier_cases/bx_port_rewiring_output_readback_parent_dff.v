// TOP: top
// TECH: nangate45
// TARGETS: output_readback, feedback, sequential
// CLUE: The parent takes a child output back into another input port of the
// CLUE: SAME child through a flop.

module top (a, clk, y);
 input a, clk;
 output y;
 wire w, q;
 sub u (.i0(a), .i1(q), .o(w));
 DFF_X1 r (.D(w), .CK(clk), .Q(q));
 BUF_X1 b (.A(w), .Z(y));
endmodule

module sub (i0, i1, o);
 input i0, i1;
 output o;
 XNOR2_X1 g (.A(i0), .B(i1), .ZN(o));
endmodule
