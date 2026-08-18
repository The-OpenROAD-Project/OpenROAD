// TOP: top
// TECH: nangate45
// TARGETS: nonansi_header_order, positional_conn, interleaved
// CLUE: Header interleaves outputs and inputs (q0, p0, q1, p1) while the body
// CLUE: declares all inputs then all outputs; instance is positional.

module top (a0, a1, y0, y1);
 input a0, a1;
 output y0, y1;
 sub u (y0, a0, y1, a1);
endmodule

module sub (q0, p0, q1, p1);
 input p0, p1;
 output q0, q1;
 INV_X1 g0 (.A(p0), .ZN(q0));
 BUF_X1 g1 (.A(p1), .Z(q1));
endmodule
