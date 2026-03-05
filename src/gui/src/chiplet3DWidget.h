// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include <QMatrix4x4>
#include <QQuaternion>
#include <QTimer>
#include <QVector2D>
#include <QVector3D>
#include <QWidget>
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "gui/gui.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"

namespace odb {
class dbChip;
class dbBlock;
}  // namespace odb
namespace utl {
class Logger;
}

namespace gui {

class Chiplet3DWidget : public QWidget
{
  Q_OBJECT

 public:
  Chiplet3DWidget(const SelectionSet& selected,
                  const HighlightSet& highlighted,
                  QWidget* parent = nullptr);

  void setChip(odb::dbChip* chip);
  void setLogger(utl::Logger* logger);

 public slots:
  void zoomTo(const Selected& selection);
  void selectionFocus(const Selected& focus);

 protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

 private:
  struct Face;
  void buildGeometries();

  // Helper to draw a line with 3D projection and clipping
  void drawLine3D(QPainter& painter,
                  const QVector3D& p1_world,
                  const QVector3D& p2_world,
                  const QColor& color,
                  const QMatrix4x4& modelView,
                  const QMatrix4x4& projection,
                  const QRect& viewport,
                  float zNear);

  void drawFace3D(QPainter& painter,
                  const Face& face,
                  const QMatrix4x4& modelView,
                  const QMatrix4x4& projection,
                  const QRect& viewport,
                  float zNear);

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
  odb::dbTransform
      center_transform_;  // identity transform; updated in buildGeometries()

  struct VertexData
  {
    QVector3D position;
    QVector3D color;
  };

  struct Face
  {
    std::array<uint32_t, 4> indices;
    QColor color;
    Selected selection;
  };

  struct ProjectedFace
  {
    const Face* face;
    float sortZ;
  };

  const SelectionSet& selected_;
  const HighlightSet& highlighted_;

  struct AnimatedSelected
  {
    Selected selection;
    int state_count;
    int max_state_count;
    int state_modulo;
    std::unique_ptr<QTimer> timer;
  };
  std::unique_ptr<AnimatedSelected> animate_selection_;
  static constexpr int kAnimationRepeats = 6;

  std::vector<VertexData> vertices_;
  std::vector<Face> faces_;
  std::vector<odb::Cuboid> chip_cuboids_;
  std::vector<ProjectedFace> sorted_faces_;

  // Reusable buffers for drawFace3D to avoid per-frame heap allocations
  std::vector<QVector3D> face_view_points_;
  std::vector<QVector3D> face_clipped_points_;
  QPolygonF polygon_buffer_;
};

}  // namespace gui
