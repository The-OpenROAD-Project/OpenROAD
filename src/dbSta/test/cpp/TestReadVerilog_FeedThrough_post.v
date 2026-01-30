module top (ass_out);
 output ass_out;


 INV_X1 U2 (.A(ass_out));
 top_impl impl (.ass_out(ass_out));
endmodule
module ASSIGN_MODULE (in,
    out);
 input in;
 output out;


 assign out = in;
endmodule
module top_impl (ass_out);
 output ass_out;

 wire ass_in;

 INV_X1 U1 (.ZN(ass_in));
 INV_X1 U2 (.A(ass_out));
 ASSIGN_MODULE ass (.in(ass_in),
    .out(ass_out));
endmodule
