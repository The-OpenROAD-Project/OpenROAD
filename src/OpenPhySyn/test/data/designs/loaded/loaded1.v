module loaded (
	in1, 
	in2,
	clk,
	out
);
	input in1;
	input in2;
	input clk;
	output [15: 0] out;

	AND2_X1 g1(
		.A1(in1),
		.A2(in2), 
		.ZN(g1z)
	);
	AND2_X1 g2(
		.A1(in1),
		.A2(in2), 
		.ZN(g2z)
	);

	AND2_X1 g3(
		.A1(g1z),
		.A2(g2z), 
		.ZN(g3z)
	);

	DFF_X1 r1 (
		.Q(out[0]),
		.D(g3z),
		.CK(clk)
	);
	DFF_X1 r2 (
		.Q(out[1]),
		.D(g3z),
		.CK(clk)
	);
	DFF_X1 r3 (
		.Q(out[2]),
		.D(g3z),
		.CK(clk)
	);
	DFF_X1 r4 (
		.Q(out[3]),
		.D(g3z),
		.CK(clk)
	);
	DFF_X1 r5 (
		.Q(out[4]),
		.D(g3z),
		.CK(clk)
	);
	DFF_X1 r6 (
		.Q(out[5]),
		.D(g3z),
		.CK(clk)
	);
	DFF_X1 r7 (
		.Q(out[6]),
		.D(g3z),
		.CK(clk)
	);
	DFF_X1 r8 (
		.Q(out[7]),
		.D(g3z),
		.CK(clk)
	);
	DFF_X1 r9 (
		.Q(out[8]),
		.D(g3z),
		.CK(clk)
	);
	DFF_X1 r10 (
		.Q(out[9]),
		.D(g3z),
		.CK(clk)
	);
	DFF_X1 r11 (
		.Q(out[10]),
		.D(g3z),
		.CK(clk)
	);
	DFF_X1 r12 (
		.Q(out[11]),
		.D(g3z),
		.CK(clk)
	);
	DFF_X1 r13 (
		.Q(out[12]),
		.D(g3z),
		.CK(clk)
	);
	DFF_X1 r14 (
		.Q(out[13]),
		.D(g3z),
		.CK(clk)
	);
	DFF_X1 r15 (
		.Q(out[14]),
		.D(g3z),
		.CK(clk)
	);
	DFF_X1 r16 (
		.Q(out[15]),
		.D(g3z),
		.CK(clk)
	);
endmodule
