module top (in1, in2, out);
  input in1, in2;
  output out;
  wire u1zn, u2zn;

  NAND2_X1 u1 (.A1(in1), .A2(1'b0), .ZN(u1zn));
  INV_X1 u2 (.A(u1zn), .ZN(u2zn));
  OR2_X1 u3 (.A1(u2zn), .A2(in2), .ZN(out));
endmodule // top
