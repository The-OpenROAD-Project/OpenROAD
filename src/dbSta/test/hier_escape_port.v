// Regression test for dbNetwork::name(Port*) escaped-identifier handling.
//
// `child` has two output ports whose Verilog escaped-identifier names
// (`\pathA/midB/leaf ` and `\pathA/midC/leaf `) differ only in a middle
// segment.  Every `/` is a literal name character of the escaped
// identifier (LRM 5.6.1) and is stored as `\/` in ODB.
//
// Before the fix, dbNetwork::name(Port*) stripped at the last `/` without
// honoring the preceding `\`, collapsing both names to `leaf` and
// producing a malformed child module with two identically-named output
// ports on write_verilog round-trip.

module top (clk);
 input clk;
 wire net_b;
 wire net_c;
 child h1 (.clk(clk),
           .\pathA/midB/leaf (net_b),
           .\pathA/midC/leaf (net_c));
endmodule

module child (clk, \pathA/midB/leaf , \pathA/midC/leaf );
 input clk;
 output \pathA/midB/leaf ;
 output \pathA/midC/leaf ;
 DFF_X1 ff_b (.CK(clk), .Q(\pathA/midB/leaf ));
 DFF_X1 ff_c (.CK(clk), .Q(\pathA/midC/leaf ));
endmodule
