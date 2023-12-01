// Copyright 2019-2023 The Regents of the University of California, Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

#include "sta/PathRef.hh"

namespace sta {

class AbstractPathRenderer
{
 public:
  virtual ~AbstractPathRenderer() = default;

  virtual void highlight(PathRef* path) = 0;
};

}  // namespace sta
