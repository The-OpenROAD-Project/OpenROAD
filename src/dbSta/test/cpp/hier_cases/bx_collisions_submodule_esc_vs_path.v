// TOP: top
// TECH: nangate45
// TARGETS: escaped_inst, flat_path_collision, collision_below_top
// CLUE: the collision is generated entirely below top: module mid contains
// escaped instance \c/d AND hierarchy c containing d; both flatten to m/c/d.
module leafm (input a, output z);
  INV_X1 d (.A(a), .ZN(z));
endmodule

module mid (input a1, input a2, output z1, output z2);
  leafm c (.a(a1), .z(z1));
  INV_X1 \c/d  (.A(a2), .ZN(z2));
endmodule

module top (input in1, input in2, output o1, output o2);
  mid m (.a1(in1), .a2(in2), .z1(o1), .z2(o2));
endmodule
