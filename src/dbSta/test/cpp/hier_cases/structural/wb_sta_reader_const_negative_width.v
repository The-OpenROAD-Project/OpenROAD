// TOP: top
// TECH: nangate45
// TARGETS: constant, negative_width, stol, reader_robustness
// CLUE: VerilogLex.ll:97-115 puts {SIGN}? in the CONSTANT token, so "-1'b1" lexes as
// CLUE: one constant.  VerilogNetConstant::parseConstant (VerilogReader.cc:1172) does
// CLUE: size_t size = std::stol("-1") -> 2^64-1 and then new vector<bool>(size).
module top (a, y);
  input a;
  output y;
  NAND2_X1 g (.A1(a), .A2(-1'b1), .ZN(y));
endmodule
