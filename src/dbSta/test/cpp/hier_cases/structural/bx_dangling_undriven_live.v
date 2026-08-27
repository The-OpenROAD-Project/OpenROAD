// TOP: top
// TECH: nangate45
// TARGETS: undriven_net_live_cone
// CLUE: declared-but-undriven wire und feeds AND2 u2 whose output IS a top
//       output yu. LEC coverage goes partial; observe both writers structurally.
module top (x1, y, yu);
  input x1;
  output y;
  output yu;
  wire und;
  INV_X1 u1 (.A(x1), .ZN(y));
  AND2_X1 u2 (.A1(x1), .A2(und), .ZN(yu));
endmodule
