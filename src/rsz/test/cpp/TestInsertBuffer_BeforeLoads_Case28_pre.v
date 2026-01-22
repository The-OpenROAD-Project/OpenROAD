// Test case for hierarchical name collision with internal signal during port punching.
// When buffer is placed at top module and punches a port into H1,
// the port name "_019_" should be avoided because H1 has an internal net with that name.
module top (in);
 input in;

 wire _019_;

 // Driver outputs to net "_019_" in top module
 BUF_X1 drvr (.A(in),
    .Z(_019_));
 // H1 receives "_019_" from top
 H1 h1 (.A(_019_));
 // load4 in top module receives "_019_" (target for buffer insertion)
 BUF_X1 load4 (.A(_019_));
endmodule

module H1 (A);
 input A;

 // "_019_" is an INTERNAL signal (xor gate output) - same name as the incoming signal
 wire _019_;

 // load1 and load3 receive input A
 BUF_X1 load1 (.A(A));
 // load2 receives internal "_019_" signal
 BUF_X1 load2 (.A(_019_));
 BUF_X1 load3 (.A(A));
 XOR2_X1 xor_gate (.A(A),
    .B(A),
    .Z(_019_));
endmodule
