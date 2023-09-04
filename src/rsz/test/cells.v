module AND2_X1 (A1, A2, ZN);
   input A1;
   input A2;
   output ZN;
   assign ZN = (A1 & A2);   
endmodule // AND2_X1

module AND2_X2 (A1, A2, ZN);
   input A1;
   input A2;
   output ZN;
   assign ZN = (A1 & A2);   
endmodule // AND2_X2

module AND2_X4 (A1, A2, ZN);
   input A1;
   input A2;
   output ZN;
   assign ZN = (A1 & A2);   
endmodule // AND2_X4

module BUF_X1 (A, Z);
   input A;
   output Z;
   assign Z = (A);
endmodule // BUF_X1

module BUF_X2 (A, Z);
   input A;
   output Z;
   assign Z = (A);
endmodule // BUF_X2

module BUF_X4 (A, Z);
   input A;
   output Z;
   assign Z = (A);
endmodule // BUF_X4

module BUF_X8 (A, Z);
   input A;
   output Z;
   assign Z = (A);
endmodule // BUF_X8

module BUF_X16 (A, Z);
   input A;
   output Z;
   assign Z = (A);
endmodule // BUF_X16

module BUF_X32 (A, Z);
   input A;
   output Z;
   assign Z = (A);
endmodule // BUF_X32

module CLKBUF_X1 (A, Z);
   input A;
   output Z;
   assign Z = (A);
endmodule // CLKBUF_X1

module CLKBUF_X2 (A, Z);
   input A;
   output Z;
   assign Z = (A);
endmodule // CLKBUF_X2

module CLKBUF_X4 (A, Z);
   input A;
   output Z;
   assign Z = (A);
endmodule // CLKBUF_X4

module DFF_X1 (CK, D, Q);
   input CK;
   input D;
   output Q;   
endmodule // DFF_X1

module DFFR_X1 (CK, D, RN, Q);
   input CK;
   input D;
   input RN;
   output Q;   
endmodule // DFF_X1

module DFF_X2 (CK, D, Q);
   input CK;
   input D;
   output Q;   
endmodule // DFF_X2

module NAND2_X1 (A1, A2, ZN);
   input A1;
   input A2;
   output ZN;
   assign ZN = !(A1 & A2);   
endmodule // NAND2_X1

module NAND2_X2 (A1, A2, ZN);
   input A1;
   input A2;
   output ZN;
   assign ZN = !(A1 & A2);   
endmodule // NAND2_X2

module NAND2_X4 (A1, A2, ZN);
   input A1;
   input A2;
   output ZN;
   assign ZN = !(A1 & A2);   
endmodule // NAND2_X4

module XNOR2_X1 (A1, A2, ZN);
   input A1;
   input A2;
   output ZN;
   assign ZN = ~(A1 ^ A2);
endmodule // XNOR_X1

module XNOR2_X2 (A1, A2, ZN);
   input A1;
   input A2;
   output ZN;
   assign ZN = ~(A1 ^ A2);
endmodule // XNOR_X2

module XNOR2_X4 (A1, A2, ZN);
   input A1;
   input A2;
   output ZN;
   assign ZN = ~(A1 ^ B2);
endmodule // XNOR_X4

