// TOP: top
// TECH: nangate45
// TARGETS: case_sensitivity, module_name, depth_3
// CLUE: Module chain top -> Mid -> MID where Mid and MID are distinct definitions differing only by case.
module MID (i, o);
  input i;
  output o;
  INV_X1 g (.A(i), .ZN(o));
endmodule
module Mid (i, o);
  input i;
  output o;
  wire w;
  MID u (.i(i), .o(w));
  BUF_X1 b (.A(w), .Z(o));
endmodule
module top (a, y);
  input a;
  output y;
  wire w;
  Mid u (.i(a), .o(w));
  INV_X1 v (.A(w), .ZN(y));
endmodule
