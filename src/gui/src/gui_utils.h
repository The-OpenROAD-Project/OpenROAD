// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <QColor>
#include <QString>
#include <QWidget>

namespace utl {
class Logger;
}

namespace gui {

class Utils
{
 public:
  static QString requestImageSavePath(QWidget* parent, const QString& title);
  static QString fixImagePath(const QString& path, utl::Logger* logger);
  static QSize adjustMaxImageSize(const QSize& size);
  static void renderImage(const QString& path,
                          QWidget* widget,
                          int width_px,
                          int height_px,
                          const QRect& render_rect,
                          const QColor& background,
                          utl::Logger* logger);
  static QString wrapInCurly(const QString& q_string);

  // Cache of size in pixels to limit ~1.5GB in memory
  static constexpr int kMaxImageSize = 7200;
};

}  // namespace gui
