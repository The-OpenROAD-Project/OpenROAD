// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <cstdint>

#include "odb/db.h"
#include "tst/fixture.h"

namespace odb {

class SimpleDbFixture : public tst::Fixture
{
 protected:
  static odb::dbMaster* createMaster2X1(odb::dbLib* lib,
                                        const char* name,
                                        uint32_t width,
                                        uint32_t height,
                                        const char* in1,
                                        const char* in2,
                                        const char* out);

  static odb::dbMaster* createMaster1X1(dbLib* lib,
                                        const char* name,
                                        uint32_t width,
                                        uint32_t height,
                                        const char* in1,
                                        const char* out);

  void createSimpleDB();

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
  void create2LevetDbNoBTerms();

  void create2LevetDbWithBTerms();
};

}  // namespace odb
