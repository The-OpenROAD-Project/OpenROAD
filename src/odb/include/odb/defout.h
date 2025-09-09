// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "odb.h"
namespace utl {
class Logger;
}
namespace odb {

class defout_impl;
class dbNet;
class dbBlock;

class defout
{
  defout_impl* _writer;

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

  defout(utl::Logger* logger);
  ~defout();

  void setUseLayerAlias(bool value);
  void setUseNetInstIds(bool value);
  void setUseMasterIds(bool value);
  void selectNet(dbNet* net);
  void setVersion(Version v);  // default is 5.8

  bool writeBlock(dbBlock* block, const char* def_file);
};

}  // namespace odb
