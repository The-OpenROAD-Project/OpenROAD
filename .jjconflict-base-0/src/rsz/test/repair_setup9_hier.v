// Defines a leaf-level module with a simple register-to-register path
// including a buffer chain. This creates a straightforward sequential path.
module leaf1 (
  input clk,
  input in1,
  output out1
);

  wire n1, n2, n3;
  DFF_X1 dff1 (.D(in1), .CK(clk), .Q(n1));
  BUF_X1 buf1 (.A(n1), .Z(n2));
  BUF_X1 buf2 (.A(n2), .Z(n3));
  DFF_X1 dff2 (.D(n3), .CK(clk), .Q(out1));

endmodule

// Defines a leaf-level module with fanout, where one signal drives
// multiple buffers and registers. This introduces path divergence.
module leaf2 (
  input clk,
  input in1,
  output out1,
  output out2
);

  wire n1, n2, n3;
  DFF_X1 dff1 (.D(in1), .CK(clk), .Q(n1));
  BUF_X1 buf1 (.A(n1), .Z(n2));
  BUF_X1 buf2 (.A(n1), .Z(n3));
  DFF_X1 dff2 (.D(n2), .CK(clk), .Q(out1));
  DFF_X1 dff3 (.D(n3), .CK(clk), .Q(out2));

endmodule

// Defines a leaf-level module with a longer buffer chain to create
// a path with more significant delay.
module leaf3 (
  input clk,
  input in1,
  output out1
);

  wire n1, n2, n3, n4, n5;
  DFF_X1 dff1 (.D(in1), .CK(clk), .Q(n1));
  BUF_X1 buf1 (.A(n1), .Z(n2));
  BUF_X1 buf2 (.A(n2), .Z(n3));
  BUF_X1 buf3 (.A(n3), .Z(n4));
  BUF_X1 buf4 (.A(n4), .Z(n5));
  DFF_X1 dff2 (.D(n5), .CK(clk), .Q(out1));

endmodule

// Defines a simple leaf-level module that registers its input.
// This serves as a basic sequential element.
module leaf4 (
  input clk,
  input in1,
  output out1
);

  DFF_X1 dff1 (.D(in1), .CK(clk), .Q(out1));

endmodule

// Defines a mid-level module that instantiates leaf1 and leaf2,
// creating longer and more complex paths between them.
module mid1 (
  input clk,
  input in1,
  input in2,
  output out1,
  output out2
);

  wire l1_out, l2_out1, l2_out2;

  leaf1 u_leaf1 (
    .clk(clk),
    .in1(in1),
    .out1(l1_out)
  );

  leaf2 u_leaf2 (
    .clk(clk),
    .in1(in2),
    .out1(l2_out1),
    .out2(l2_out2)
  );

  DFF_X1 dff_out1 (.D(l1_out), .CK(clk), .Q(out1));
  DFF_X1 dff_out2 (.D(l2_out1), .CK(clk), .Q(out2));
  DFF_X1 dff_load1 (.D(l2_out1), .CK(clk), .Q());
  DFF_X1 dff_load2 (.D(l2_out1), .CK(clk), .Q());
  DFF_X1 dff_load3 (.D(l2_out1), .CK(clk), .Q());
  DFF_X1 dff_load4 (.D(l2_out1), .CK(clk), .Q());
  DFF_X1 dff_load5 (.D(l2_out1), .CK(clk), .Q());
  DFF_X1 dff_load6 (.D(l2_out1), .CK(clk), .Q());
  DFF_X1 dff_load7 (.D(l2_out1), .CK(clk), .Q());
  DFF_X1 dff_load8 (.D(l2_out1), .CK(clk), .Q());
  DFF_X1 dff_load9 (.D(l2_out1), .CK(clk), .Q());
  DFF_X1 dff_load10 (.D(l2_out1), .CK(clk), .Q());

endmodule

// Defines another mid-level module that instantiates leaf3 and leaf4,
// combining their paths.
module mid2 (
  input clk,
  input in1,
  input in2,
  output out1
);

  wire l3_out, l4_out;

  leaf3 u_leaf3 (
    .clk(clk),
    .in1(in1),
    .out1(l3_out)
  );

  leaf4 u_leaf4 (
    .clk(clk),
    .in1(in2),
    .out1(l4_out)
  );

  wire n1;
  BUF_X1 buf1 (.A(l3_out), .Z(n1));
  DFF_X1 dff_out1 (.D(n1), .CK(clk), .Q(out1));

endmodule

// Defines the top-level module that instantiates the mid-level modules,
// creating the final hierarchical structure with various timing paths.
module top (
  input clk,
  input in1,
  input in2,
  input in3,
  input in4,
  output out1,
  output out2
);

  wire m1_out1, m1_out2, m2_out1;

  mid1 u_mid1 (
    .clk(clk),
    .in1(in1),
    .in2(in2),
    .out1(m1_out1),
    .out2(m1_out2)
  );

  mid2 u_mid2 (
    .clk(clk),
    .in1(in3),
    .in2(in4),
    .out1(m2_out1)
  );

  wire n1, n2;
  BUF_X1 buf1 (.A(m1_out1), .Z(n1));
  BUF_X1 buf2 (.A(m2_out1), .Z(n2));

  DFF_X1 dff_out1 (.D(n1), .CK(clk), .Q(out1));
  DFF_X1 dff_out2 (.D(n2), .CK(clk), .Q(out2));

endmodule