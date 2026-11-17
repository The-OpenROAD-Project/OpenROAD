/*
 FeedThrough test.
 
 Test to catch the case when an intermediate module has
 a feedthrough path (assign out = in)
 */
module top(ass_out);
   output ass_out;
   top_impl impl (.ass_out(ass_out));
   INV_X1 U2 (.A(ass_out), .ZN());
endmodule // top

module top_impl(ass_out);
output ass_out;
wire ass_in;
INV_X1 U1 (.A(), .ZN(ass_in));
INV_X1 U2 (.A(ass_out), .ZN());
ASSIGN_MODULE ass (.in(ass_in), .out(ass_out));
endmodule // top_impl

module ASSIGN_MODULE(in, out);
input in;
output out;
assign out = in ;
endmodule
