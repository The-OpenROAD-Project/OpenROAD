// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors
#pragma once
#include <QMatrix4x4>
#include <QPolygonF>
#include <QQuaternion>
#include <QTimer>
#include <QVector2D>
#include <QVector3D>
#include <QWidget>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
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
  ~Chiplet3DWidget() override;
  void setChip(odb::dbChip* chip);
  void setLogger(utl::Logger* logger);
 public slots:
  void zoomTo(const Selected& selection);
  void selectionFocus(const Selected& focus);

 signals:
  void selected(const Selected& selection);

 protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

 private:
  struct Face;
  struct ChipletObject;
  void buildGeometries();
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
  std::optional<QPolygonF> projectAndClipFace(const Face& face,
                                              const QMatrix4x4& modelView,
                                              const QMatrix4x4& projection,
                                              float width,
                                              float height,
                                              float kClipZ) const;
  odb::dbChip* chip_ = nullptr;
  utl::Logger* logger_ = nullptr;
  QVector2D mouse_press_position_;
  QQuaternion rotation_;
  // Camera State
  float distance_ = 0.0f;
  float pan_x_ = 0.0f;
  float pan_y_ = 0.0f;
  float bounding_radius_ = 0.0f;
  odb::dbTransform center_transform_;
  struct VertexData
  {
    QVector3D position;
  };
  struct Face
  {
    std::array<uint32_t, 4> indices;
    QColor color;
    Selected selection;
    int chiplet_index;  // which chiplet this face belongs to
  };
  // Groups all 6 faces of a single chiplet for per-object sorting
  struct ChipletObject
  {
    std::vector<int> face_indices;  // indices into faces_
    float world_z_min;              // world-space logical Z layer
    float world_z_max;
    float world_x_min;
    float world_x_max;
    float world_y_min;
    float world_y_max;
  };

  struct ProjectedFace
  {
    const Face* face;
    double sortZ;  // view-space depth for sorting
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
  std::vector<ChipletObject> chiplet_objects_;
  std::vector<odb::Cuboid> chip_cuboids_;

  std::vector<ProjectedFace> sorted_faces_;
};
}  // namespace gui
