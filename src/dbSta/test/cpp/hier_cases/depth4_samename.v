// TOP: top
// TECH: nangate45
// TARGETS: depth_4, feedthrough_same_name
// CLUE: depth-4 feedthrough where every level reuses identical port names a/y and inner net mid; uniquification stress for flat write.

module t4 (input a, output y);
  assign y = a;
endmodule

module t3 (input a, output y);
  wire mid;
  t4 u (.a(a), .y(mid));
  assign y = mid;
endmodule

module t2 (input a, output y);
  wire mid;
  t3 u (.a(a), .y(mid));
  assign y = mid;
endmodule

module t1 (input a, output y);
  wire mid;
  t2 u (.a(a), .y(mid));
  assign y = mid;
endmodule

module top (input a, input zi, output y, output zo);
  wire mid;
  t1 u (.a(a), .y(mid));
  assign y = mid;
  INV_X1 g_anchor (.A(zi), .ZN(zo));
endmodule
