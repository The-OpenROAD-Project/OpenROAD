// TOP: top
// TECH: nangate45
// TARGETS: positional_conn, leaf_cell, depth_2
// CLUE: Positional leaf-cell connection two levels down, where the parent path
// CLUE: also permutes bus bits: does the mis-binding follow the cell wherever it
// CLUE: sits?

module top (a, b, y);
 input a, b;
 output y;
 mid u (.p(b), .q(a), .z(y));
endmodule

module mid (p, q, z);
 input p, q;
 output z;
 leafm u (.p(q), .q(p), .z(z));
endmodule

module leafm (p, q, z);
 input p, q;
 output z;
 XOR2_X1 g (p, q, z);
endmodule
