// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include "color.h"

namespace web {

// 31-color palette for module coloring.  Assigned to modules in DFS
// order by HierarchyReport::getReport().
//
// The same palette is duplicated in the Qt GUI's ColorGenerator
// (src/gui/src/colorGenerator.cpp).  Keep them in sync.
constexpr int kModuleColorPaletteSize = 31;

// NOLINTBEGIN(modernize-use-designated-initializers)
constexpr Color kModuleColorPalette[kModuleColorPaletteSize] = {
    {255, 0, 0, 100},     {255, 140, 0, 100},   {255, 215, 0, 100},
    {0, 255, 0, 100},     {148, 0, 211, 100},   {0, 250, 154, 100},
    {220, 20, 60, 100},   {0, 255, 255, 100},   {0, 191, 255, 100},
    {0, 0, 255, 100},     {173, 255, 47, 100},  {218, 112, 214, 100},
    {255, 0, 255, 100},   {30, 144, 255, 100},  {250, 128, 114, 100},
    {176, 224, 230, 100}, {255, 20, 147, 100},  {123, 104, 238, 100},
    {255, 250, 205, 100}, {255, 182, 193, 100}, {85, 107, 47, 100},
    {139, 69, 19, 100},   {72, 61, 139, 100},   {0, 128, 0, 100},
    {60, 179, 113, 100},  {184, 134, 11, 100},  {0, 139, 139, 100},
    {0, 0, 139, 100},     {50, 205, 50, 100},   {128, 0, 128, 100},
    {176, 48, 96, 100},
};
// NOLINTEND(modernize-use-designated-initializers)

}  // namespace web
