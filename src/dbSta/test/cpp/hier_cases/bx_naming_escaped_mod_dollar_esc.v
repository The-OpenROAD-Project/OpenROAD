// TOP: top
// TECH: nangate45
// TARGETS: escaped_module, char_dollar_leading, depth_2
// CLUE: module named \$sub ; leading dollar is only legal via escaping,
// writers that test "needs escape?" by scanning for specials may miss the
// leading-$ rule.
module \$sub (input a, output z);
  INV_X1 g1 (.A(a), .ZN(z));
endmodule
module top (input a, output z);
  \$sub u1 (.a(a), .z(z));
endmodule
