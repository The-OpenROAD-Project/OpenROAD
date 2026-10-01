// TOP: top
// TECH: nangate45
// TARGETS: hier, no_hierarchy, flattened_netlist_shape, name_erase_overshoot, short
// CLUE: the realistic shape of the erase-overshoot. Commercial synthesis emits
// flattened netlists whose instance AND net names are escaped hierarchical paths
// (`\u1/g1 `, `\u1/n `). For driver `\u1/g1 ` (sta "u1\/g1") name(Net)'s
// header_to_remove is "u1\", which IS a prefix of the net's sta name "u1\/n", so 4
// chars are erased and `\u1/n ` collapses to plain "n". Every flattened instance
// suffixes the same local net names, so `\u1/n ` and `\u2/n ` should both collapse
// to "n": one wire name for two independent cones.
module top (a, b, y0, y1);
   input a;
   input b;
   output y0;
   output y1;
   wire \u1/n ;
   wire \u2/n ;
   INV_X1 \u1/g1  (.A(a), .ZN(\u1/n ));
   BUF_X1 \u1/g2  (.A(\u1/n ), .Z(y0));
   INV_X1 \u2/g1  (.A(b), .ZN(\u2/n ));
   BUF_X1 \u2/g2  (.A(\u2/n ), .Z(y1));
endmodule
