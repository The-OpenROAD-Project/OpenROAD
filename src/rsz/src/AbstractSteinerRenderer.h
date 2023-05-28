// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

namespace rsz {

class SteinerTree;

class AbstractSteinerRenderer
{
 public:
  virtual ~AbstractSteinerRenderer() = default;
  virtual void highlight(SteinerTree* tree) = 0;
};

}  // namespace rsz
