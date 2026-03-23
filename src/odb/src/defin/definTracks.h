// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <vector>

#include "definBase.h"
#include "definTypes.h"

namespace odb {

struct Track
{
  int _dir;
  int _orig;
  int _step;
  int _count;
  int _first_mask;
  bool _samemask;
};

class definTracks : public definBase
{
  Track _track;

 public:
  virtual void tracksBegin(defDirection dir,
                           int orig,
                           int count,
                           int step,
                           int first_mask,
                           bool samemask);
  virtual void tracksLayer(const char* layer);
  virtual void tracksEnd();
};

}  // namespace odb
