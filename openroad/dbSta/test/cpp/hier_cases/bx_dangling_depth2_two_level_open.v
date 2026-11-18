// TOP: top
// TECH: nangate45
// TARGETS: dangling_input, depth_2, feedthrough_port, empty_named_conn, dead_cone
// CLUE: mid input q feeds leaf input p (dead cone); top leaves mid.q explicitly empty.
// Unconnected chain through two hierarchy levels — does q->p wiring survive?
module leaf (input a, input p, output y);
  wire pd;
  INV_X1 g1 (.A(a), .ZN(y));
  INV_X1 g2 (.A(p), .ZN(pd));
endmodule
module mid (input a, input q, output y);
  leaf u_l (.a(a), .p(q), .y(y));
endmodule
module top (input in1, output out1);
  mid u_m (.a(in1), .q(), .y(out1));
endmodule
