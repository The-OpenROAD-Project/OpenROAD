module top (in1, in2, clk1, clk2, clk3, out);
    input in1, in2, clk1, clk2, clk3;
    output out;
    wire r1q, r2q, u1z, u2z;
    MOCK_HYBRID_A r1 (.A1(in1), .A2(clk1), .ZN(r1q));
    MOCK_HYBRID_G u2 (.A1(r1q), .A2(u1z), .ZN(u2z));
    MOCK_HYBRID_AG r2 (.A1(in2), .A2(clk2), .ZN(r2q));
    MOCK_HYBRID_G u1 (.A1(r2q), .A2(u2z), .ZN(u1z));
    MOCK_HYBRID_AG u3 (.A1(u1z), .A2(clk3), .ZN(out));
endmodule // top
