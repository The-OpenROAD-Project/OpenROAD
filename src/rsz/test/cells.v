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

module NAND2_X4 (A1, A2, ZN);
 input A1;
 input A2;
   output ZN;
   assign ZN = !(A1 & A2);   
endmodule // NAND2_X4
