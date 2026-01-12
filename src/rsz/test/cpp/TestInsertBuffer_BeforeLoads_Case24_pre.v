// Case24: Reproduction of ORD-2030 - Multiple ModNets in target module
//
// Hierarchy:
// - top (target module where buffer will be placed)
//   - H0 (child module with feedthrough)
//     - internal_buf (load inside H0)
//   - H1 (another child module)
//     - internal_load (another load inside H1)
//   - drvr (driver in top)
//
// Signal flow:
// - drvr/Z drives net 'n1' (modnet 'n1' in top)
// - n1 connects to h0/in (creates modnet 'in' inside H0)
// - h0/out (feedthrough) connects to 'w1' (modnet 'w1' in top)
// - w1 connects to h1/in (load path through different modnet)
//
// The key issue:
// - Both 'n1' and 'w1' are modnets in 'top' module
// - Both map to the same flat net through H0's feedthrough
// - When buffering loads inside H0 and H1, the algorithm must select
//   the driver's modnet ('n1'), not the load's modnet ('w1')

module top (in);
 input in;

 wire n1;
 wire w1;

 BUF_X1 drvr (.A(in), .Z(n1));
 
 // H0 has feedthrough: in -> internal_buf -> out
 // This creates two modnets in 'top' for the same flat net path
 H0 h0 (.in(n1), .out(w1));
 
 // H1 receives signal via w1 modnet (different from n1 modnet)
 H1 h1 (.in(w1));
endmodule

module H0 (in, out);
 input in;
 output out;

 BUF_X1 internal_buf (.A(in));
 assign out = in;
endmodule

module H1 (in);
 input in;

 // Load inside H1
 BUF_X1 internal_load (.A(in));
endmodule
