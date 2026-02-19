// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector2D>
#include <QVector3D>
#include <QWidget>
#include <cstdint>
#include <vector>

namespace odb {
class dbChip;
}
namespace utl {
class Logger;
}

namespace gui {

class Chiplet3DWidget : public QWidget
{
  Q_OBJECT

 public:
  explicit Chiplet3DWidget(QWidget* parent = nullptr);

  void setChip(odb::dbChip* chip);
  void setLogger(utl::Logger* logger);

 protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

 private:
  void buildGeometries();

  // Helper to draw a line with 3D projection and clipping
  void drawLine3D(QPainter& painter,
                  const QVector3D& p1_world,
                  const QVector3D& p2_world,
                  const QColor& color,
                  const QMatrix4x4& modelView,
                  const QMatrix4x4& projection,
                  const QRect& viewport);

  odb::dbChip* chip_ = nullptr;
  utl::Logger* logger_ = nullptr;

  QVector2D mouse_press_position_;
  QQuaternion rotation_;

  // Camera State
  float distance_ = 10.0f;
  float pan_x_ = 0.0f;
  float pan_y_ = 0.0f;

  float bounding_radius_ = 10.0f;  // Rotation-invariant bounding sphere radius
  QVector3D center_ = QVector3D(0, 0, 0);

  struct VertexData
  {
    QVector3D position;
    QVector3D color;
  };

  std::vector<VertexData> vertices_;
  std::vector<uint32_t> indices_lines_;
};

}  // namespace gui
