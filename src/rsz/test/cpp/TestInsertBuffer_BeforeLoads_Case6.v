module top;

 wire n1;

 // H2 contains the driver and load5
 ModH2 u_h2 (.out(n1));

 // Top-level load
 BUF_X1 load0 (.A(n1));

 // H3 contains load1 and load2
 ModH3 u_h3 (.in(n1));

 // H4 contains load4 and H5(load3)
 ModH4 u_h4 (.in(n1));

endmodule

module ModH2 (out);
 output out;

 wire n_internal;

 // Connect internal net to output port
 assign out = n_internal;

 // H0 contains the driver
 ModH0 u_h0 (.out(n_internal));

 // H1 contains load5 (Target)
 ModH1 u_h1 (.in(n_internal));

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
 BUF_X1 load5 (.A(in));

endmodule

module ModH3 (in);
 input in;

 // Non-target Load
 BUF_X1 load1 (.A(in));
 
 // Target Load
 BUF_X1 load2 (.A(in));

endmodule

module ModH4 (in);
 input in;

 // Non-target Load
 BUF_X1 load4 (.A(in));

 // H5 contains load3
 ModH5 u_h5 (.in(in));

endmodule

module ModH5 (in);
 input in;

 // Target Load
 BUF_X1 load3 (.A(in));

endmodule

