// TOP: top
// TECH: nangate45
// TARGETS: escaped_port, keyword_output, depth_2
// CLUE: submodule port named \output  connected by name; the hier writer
// must emit .\output ( ) without losing the escape.
module subk (input a, output \output );
  INV_X1 g1 (.A(a), .ZN(\output ));
endmodule
module top (input a, output z);
  subk u1 (.a(a), .\output (z));
endmodule
