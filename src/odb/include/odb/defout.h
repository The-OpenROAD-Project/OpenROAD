// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>

namespace utl {
class Logger;
}

namespace odb {

class dbInst;
class dbNet;
class dbBlock;

class DefOut
{
 public:
  enum Version
  {
    DEF_5_3,
    DEF_5_4,
    DEF_5_5,
    DEF_5_6,
    DEF_5_7,
    DEF_5_8
  };

  DefOut(utl::Logger* logger);
  ~DefOut();

  void selectNet(dbNet* net);
  void selectInst(dbInst* inst);

  void setVersion(Version v);  // default is 5.8

  bool writeBlock(dbBlock* block, const char* def_file);

 private:
  class Impl;
  std::unique_ptr<Impl> writer_;
};

}  // namespace odb
