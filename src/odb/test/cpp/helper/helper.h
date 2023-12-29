#include "db.h"

odb::dbMaster* createMaster2X1(odb::dbLib* lib,
                               const char* name,
                               uint width,
                               uint height,
                               const char* in1,
                               const char* in2,
                               const char* out);

odb::dbDatabase* createSimpleDB();

// #     (n1)   +-----
// #    --------|a    \    (n5)
// #     (n2)   | (i1)o|-----------+
// #    --------|b    /            |       +-------
// #            +-----             +--------\a     \    (n7)
// #                                         ) (i3)o|---------------
// #     (n3)   +-----             +--------/b     /
// #    --------|a    \    (n6)    |       +-------
// #     (n4)   | (i2)o|-----------+
// #    --------|b    /
// #            +-----
odb::dbDatabase* create2LevetDbNoBTerms();

odb::dbDatabase* create2LevetDbWithBTerms();
