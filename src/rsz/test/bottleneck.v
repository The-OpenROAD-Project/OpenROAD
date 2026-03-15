module top(in, mask, out);
input [7:0] in;
input [2:0] mask;
output [2:0] out;
wire w1, w2, w3, w4, w5, w6, w7;
XOR2_X1 _1_ (
	.A(in[0]),
	.B(in[1]),
	.Z(w1)
);
XOR2_X1 _2_ (
	.A(in[2]),
	.B(in[3]),
	.Z(w2)
);
XOR2_X1 _3_ (
	.A(in[4]),
	.B(in[5]),
	.Z(w3)
);
XOR2_X1 _4_ (
	.A(in[6]),
	.B(in[7]),
	.Z(w4)
);
XOR2_X1 _5_ (
	.A(w1),
	.B(w2),
	.Z(w5)
);
XOR2_X1 _6_ (
	.A(w3),
	.B(w4),
	.Z(w6)
);
XOR2_X1 _7_ (
	.A(w5),
	.B(w6),
	.Z(w7)
);
AND2_X1 _8_ (
	.A1(mask[0]),
	.A2(w7),
	.ZN(out[0])
);
AND2_X1 _9_ (
	.A1(mask[1]),
	.A2(w7),
	.ZN(out[1])
);
AND2_X1 _10_ (
	.A1(mask[2]),
	.A2(w7),
	.ZN(out[2])
);
endmodule
