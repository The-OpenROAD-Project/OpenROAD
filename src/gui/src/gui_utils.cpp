// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "gui_utils.h"

#include <QColor>
#include <QFileDialog>
#include <QImageWriter>
#include <QPainter>
#include <QString>
#include <QWidget>
#include <algorithm>

#include "utl/Logger.h"

namespace gui {

QString Utils::requestImageSavePath(QWidget* parent, const QString& title)
{
  QList<QByteArray> valid_extensions = QImageWriter::supportedImageFormats();

  QString images_filter = "Images (";
  for (const QByteArray& ext : valid_extensions) {
    images_filter += "*." + ext + " ";
  }
  images_filter += ")";

  return QFileDialog::getSaveFileName(parent, title, "", images_filter);
}

QString Utils::fixImagePath(const QString& path, utl::Logger* logger)
{
  QList<QByteArray> valid_extensions = QImageWriter::supportedImageFormats();

  QString fixed_path = path;

  if (!std::ranges::any_of(
          valid_extensions,

          [path](const QString& ext) { return path.endsWith("." + ext); })) {
    fixed_path += ".png";
    if (logger != nullptr) {
      logger->warn(
          utl::GUI,
          10,
          "File path does not end with a valid extension, new path is: {}",
          fixed_path.toStdString());
    }
  }

  return fixed_path;
}

QSize Utils::adjustMaxImageSize(const QSize& size)
{
  int width_px = size.width();
  int height_px = size.height();

  if (std::max(width_px, height_px) >= kMaxImageSize) {
    if (width_px > height_px) {
      const double ratio = static_cast<double>(height_px) / width_px;
      width_px = kMaxImageSize;
      height_px = ratio * kMaxImageSize;
    } else {
      const double ratio = static_cast<double>(width_px) / height_px;
      height_px = kMaxImageSize;
      width_px = ratio * kMaxImageSize;
    }
  }
  return QSize(width_px, height_px);
}

void Utils::renderImage(const QString& path,
                        QWidget* widget,
                        int width_px,
                        int height_px,
                        const QRect& render_rect,
                        const QColor& background,
                        utl::Logger* logger)
{
  const QSize img_size = adjustMaxImageSize(QSize(width_px, height_px));
  QImage img(img_size, QImage::Format_ARGB32_Premultiplied);
  if (!img.isNull()) {
    const qreal render_ratio
        = static_cast<qreal>(std::max(img_size.width(), img_size.height()))
          / std::max(render_rect.width(), render_rect.height());
    QPainter painter(&img);
    painter.scale(render_ratio, render_ratio);
    img.fill(background);

    widget->render(&painter, {0, 0}, render_rect);
    if (!img.save(path) && logger != nullptr) {
      logger->warn(
          utl::GUI, 11, "Failed to write image: {}", path.toStdString());
    }
  } else {
    if (logger != nullptr) {
      logger->warn(utl::GUI,
                   12,
                   "Image size is not valid: {}px x {}px",
                   width_px,
                   height_px);
    }
  }
}

QString Utils::wrapInCurly(const QString& q_string)
{
  return "{" + q_string + "}";
}

}  // namespace gui
