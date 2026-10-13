// TOP: top
// TECH: nangate45
// TARGETS: port_named_like_own_bus_sentinel
// CLUE: a hierarchical port `A[1:0]` yields ModBTerms A[0], A[1] plus an
// aggregate sentinel named A, and this module also declares a scalar port whose
// name shares that prefix -- so the reader must keep the sentinel and the
// scalar distinct. Exercises makeModBTerms/makeModITerms bus handling.
//
// NOTE: this originally cited dbNetwork.cc checkSanityModInstTerms, which a
// coverage run showed is executed ZERO times by the whole corpus in either link
// mode -- that sanity checker is not on the read/link/write path at all, so it
// cannot be what protects this construct. Corrected rather than deleted, since
// the coverage numbers above confirm the netlist does reach the bus-port code.
module top (a, y);
   input [1:0] a;
   output [1:0] y;
   sub u_sub (.A(a), .A_scalar(a[0]), .Y(y));
endmodule

module sub (A, A_scalar, Y);
   input [1:0] A;
   input A_scalar;
   output [1:0] Y;
   BUF_X1 g0 (.A(A[0]), .Z(Y[0]));
   AND2_X1 g1 (.A1(A[1]), .A2(A_scalar), .ZN(Y[1]));
endmodule
