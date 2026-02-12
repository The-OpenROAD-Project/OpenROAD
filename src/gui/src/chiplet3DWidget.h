// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include <QMatrix4x4>
#include <QOpenGLWidget>
#include <QQuaternion>
#include <QVector2D>
#include <QVector3D>
#include <cstdint>
#include <vector>

namespace odb {
class dbChip;
}

namespace utl {
class Logger;
}

namespace gui {

class Chiplet3DWidget : public QOpenGLWidget
{
  Q_OBJECT

 public:
  explicit Chiplet3DWidget(QWidget* parent = nullptr);
  ~Chiplet3DWidget() override = default;

  void setChip(odb::dbChip* chip);
  void setLogger(utl::Logger* logger);

 protected:
  void initializeGL() override;
  void paintGL() override;

  void mousePressEvent(QMouseEvent* e) override;
  void mouseReleaseEvent(QMouseEvent* e) override;
  void mouseMoveEvent(QMouseEvent* e) override;
  void wheelEvent(QWheelEvent* e) override;

 private:
  void buildGeometries();

  odb::dbChip* chip_ = nullptr;
  utl::Logger* logger_ = nullptr;

  QVector2D mouse_press_position_;
  QQuaternion rotation_;

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
  std::vector<uint16_t> indices_lines_;
};

}  // namespace gui
