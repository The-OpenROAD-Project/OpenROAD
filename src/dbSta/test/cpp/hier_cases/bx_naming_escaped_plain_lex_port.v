// TOP: top
// TECH: nangate45
// TARGETS: escaped_plain_equiv, port, depth_1
// CLUE: top port written escaped \pq though it lexes as plain pq
module top (\pq , z);
  input \pq ;
  output z;
  INV_X1 u1 (.A(\pq ), .ZN(z));
endmodule
