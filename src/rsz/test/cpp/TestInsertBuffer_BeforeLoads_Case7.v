module top;

 wire n1;

// H0/drvr
 ModH0 u_h0 (.out(n1));

 // Top-level loads
 BUF_X1 load0 (.A(n1));
 BUF_X1 load1 (.A(n1));
 BUF_X1 load4 (.A(n1));

 // H1/load2
 ModH1 u_h1 (.in(n1));

 // H2/H3/load3
 ModH2 u_h2 (.in(n1));

endmodule

module ModH0 (out);
 output out;

 // Driver Instance
 // Note: Input A is unconnected in the original C++ test case
 LOGIC0_X1 drvr (.Z(out));

endmodule

module ModH1 (in);
 input in;

 // Target Load
 BUF_X1 load2 (.A(in));

endmodule

module ModH2 (in);
 input in;

 // H1 contains load5 (Target)
 ModH3 u_h3 (.in(in));

endmodule

module ModH3 (in);
 input in;

 // Target Load
 BUF_X1 load3 (.A(in));

endmodule
