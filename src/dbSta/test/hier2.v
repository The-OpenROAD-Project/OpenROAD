/*
 lef: example1.lef
 lib: example1_typ.lib
 
 */

module gate1 (a1,a2,zn);
   input a1;
   input a2;
   output zn;

   AND2_X1 _5_ (
    .A1(a1),
    .A2(a2),
    .ZN(zn)
  );
   
endmodule // gatel

module top (a,b, out);
   input a;
   input b;
   output out;

   
   wire   a_int;
   
  INV_X1 _4_ (
    .A(a),
    .ZN(a_int)
  );
   
//   gate1 gate1_inst (
//	      .a1(a_int),
//	      .a2(b),
//	      .zn(out)
//	      );

   gate1 gate2_inst (
	      .a1(a_int),
	      .a2(b),
	      .zn(out)
	      );



endmodule
