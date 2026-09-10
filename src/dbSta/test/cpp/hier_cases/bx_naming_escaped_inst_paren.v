// TOP: top
// TECH: nangate45
// TARGETS: escaped_instance, char_paren, depth_1
// CLUE: instance named \(u1) ; unescaped parens collide with the port
// connection list that immediately follows the instance name.
module top (input a, output z);
  BUF_X1 \(u1) (.A(a), .Z(z));
endmodule
