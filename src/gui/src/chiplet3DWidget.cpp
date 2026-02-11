// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include "chiplet3DWidget.h"

#include <GL/gl.h>

#include <QMouseEvent>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"
#include "odb/unfoldedModel.h"
#include "utl/Logger.h"

namespace {
constexpr float INITIAL_DISTANCE_FACTOR = 3.0f;
constexpr float LAYER_GAP_FACTOR = 2.0f;
constexpr float MIN_DISTANCE_CHECK = 100.0f;
constexpr float DEFAULT_DISTANCE = 1000.0f;
constexpr float DEFAULT_SAFE_SIZE = 1000.0f;
constexpr float NEAR_FAR_FACTOR = 2.0f;
constexpr float MIN_Z_NEAR = 10.0f;
constexpr float MAX_Z_FAR_OFFSET = 10000.0f;
constexpr float GRID_SIZE_FACTOR = 1.5f;
constexpr float GRID_STEPS = 5.0f;
constexpr float GRID_Z_OFFSET_FACTOR = 0.05f;
constexpr int GRID_LINE_COUNT = 5;
constexpr float ROTATION_SENSITIVITY = 2.0f;
constexpr float PAN_SENSITIVITY = 0.002f;
constexpr float ZOOM_IN_FACTOR = 0.9f;
constexpr float ZOOM_OUT_FACTOR = 1.1f;
static const std::array<QVector3D, 7> COLOR_PALETTE = {{{0.0f, 1.0f, 0.0f},
                                                        {1.0f, 1.0f, 0.0f},
                                                        {0.0f, 1.0f, 1.0f},
                                                        {1.0f, 0.0f, 1.0f},
                                                        {1.0f, 0.5f, 0.0f},
                                                        {0.5f, 0.5f, 1.0f},
                                                        {1.0f, 0.0f, 0.0f}}};
}  // namespace

namespace gui {

Chiplet3DWidget::Chiplet3DWidget(QWidget* parent) : QOpenGLWidget(parent)
{
  // No initial rotation
}

void Chiplet3DWidget::setChip(odb::dbChip* chip)
{
  chip_ = chip;
  buildGeometries();
  update();
}

void Chiplet3DWidget::setLogger(utl::Logger* logger)
{
  logger_ = logger;
}

void Chiplet3DWidget::initializeGL()
{
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  glLineWidth(2.0f);
}

void Chiplet3DWidget::buildGeometries()
{
  if (!chip_) {
    return;
  }
  odb::Cuboid global_cuboid = chip_->getCuboid();
  odb::UnfoldedModel model(logger_, chip_);
  odb::dbTransform center_transform
      = odb::dbTransform(odb::Point3D(-global_cuboid.xCenter(),
                                      -global_cuboid.yCenter(),
                                      -global_cuboid.zCenter()));

  vertices_.clear();
  indices_lines_.clear();

  // Center and Camera
  float cx = (global_cuboid.xMin() + global_cuboid.xMax()) / 2.0f;
  float cy = (global_cuboid.yMin() + global_cuboid.yMax()) / 2.0f;
  center_ = QVector3D(cx, cy, 0.0f);

  // Calculate bounding sphere radius (half-diagonal of bounding box)
  // This remains constant regardless of rotation
  float dx = global_cuboid.dx();
  float dy = global_cuboid.dy();
  float dz = global_cuboid.dz() * LAYER_GAP_FACTOR;  // with gap factor
  bounding_radius_ = std::sqrt(dx * dx + dy * dy + dz * dz) / 2.0f;

  distance_ = bounding_radius_ * INITIAL_DISTANCE_FACTOR;
  if (distance_ < MIN_DISTANCE_CHECK) {
    distance_ = DEFAULT_DISTANCE;
  }

  int index = 0;
  for (const auto& chip : model.getChips()) {
    odb::Cuboid draw_cuboid = chip.cuboid;
    center_transform.apply(draw_cuboid);
    // Color by Depth (proportional to Z)
    QVector3D color = COLOR_PALETTE[index++ % COLOR_PALETTE.size()];

    int base = vertices_.size();
    for (const auto& p : draw_cuboid.getPoints()) {
      vertices_.push_back({QVector3D(p.x(), p.y(), p.z()), color});
    }

    indices_lines_.push_back(base + 0);
    indices_lines_.push_back(base + 1);
    indices_lines_.push_back(base + 1);
    indices_lines_.push_back(base + 2);
    indices_lines_.push_back(base + 2);
    indices_lines_.push_back(base + 3);
    indices_lines_.push_back(base + 3);
    indices_lines_.push_back(base + 0);

    indices_lines_.push_back(base + 4);
    indices_lines_.push_back(base + 5);
    indices_lines_.push_back(base + 5);
    indices_lines_.push_back(base + 6);
    indices_lines_.push_back(base + 6);
    indices_lines_.push_back(base + 7);
    indices_lines_.push_back(base + 7);
    indices_lines_.push_back(base + 4);

    indices_lines_.push_back(base + 0);
    indices_lines_.push_back(base + 4);
    indices_lines_.push_back(base + 1);
    indices_lines_.push_back(base + 5);
    indices_lines_.push_back(base + 2);
    indices_lines_.push_back(base + 6);
    indices_lines_.push_back(base + 3);
    indices_lines_.push_back(base + 7);
  }
}

void Chiplet3DWidget::paintGL()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glMatrixMode(GL_PROJECTION);
  QMatrix4x4 proj;

  // Use bounding_radius_ which is rotation-invariant
  float safe_size = bounding_radius_;
  if (safe_size < 1.0f) {
    safe_size = DEFAULT_SAFE_SIZE;
  }

  float zNear = distance_ - safe_size * NEAR_FAR_FACTOR;
  float zFar = distance_ + safe_size * NEAR_FAR_FACTOR;

  zNear = std::max(zNear, MIN_Z_NEAR);
  if (zFar < zNear + MIN_DISTANCE_CHECK) {
    zFar = zNear + MAX_Z_FAR_OFFSET;
  }

  qreal aspect = qreal(width()) / qreal(height() ? height() : 1);
  proj.perspective(45.0f, aspect, zNear, zFar);
  glLoadMatrixf(proj.constData());

  glMatrixMode(GL_MODELVIEW);
  QMatrix4x4 matrix;
  matrix.translate(pan_x_, pan_y_, -distance_);  // Apply Pan
  matrix.rotate(rotation_);                      // Apply Rotation
  glLoadMatrixf(matrix.constData());

  // Draw Reference Grid
  glBegin(GL_LINES);
  glColor3f(0.3f, 0.3f, 0.3f);
  float grid_size = safe_size * GRID_SIZE_FACTOR;  // Adjust grid to scene size
  float step = grid_size / GRID_STEPS;
  float grid_z = -center_.z() - (distance_ * GRID_Z_OFFSET_FACTOR * 0.5f);

  for (int i = -GRID_LINE_COUNT; i <= GRID_LINE_COUNT; ++i) {
    float pos = i * step;
    glVertex3f(pos, -grid_size, grid_z);
    glVertex3f(pos, grid_size, grid_z);
    glVertex3f(-grid_size, pos, grid_z);
    glVertex3f(grid_size, pos, grid_z);
  }
  glEnd();

  if (vertices_.empty()) {
    return;
  }

  glBegin(GL_LINES);
  for (uint16_t idx : indices_lines_) {
    if (idx < vertices_.size()) {
      const auto& v = vertices_[idx];
      glColor3f(v.color.x(), v.color.y(), v.color.z());
      glVertex3f(v.position.x(), v.position.y(), v.position.z());
    }
  }
  glEnd();
}

void Chiplet3DWidget::mousePressEvent(QMouseEvent* e)
{
  mouse_press_position_ = QVector2D(e->localPos());
}

void Chiplet3DWidget::mouseReleaseEvent(QMouseEvent* e)
{
}

void Chiplet3DWidget::mouseMoveEvent(QMouseEvent* e)
{
  QVector2D diff = QVector2D(e->localPos()) - mouse_press_position_;
  mouse_press_position_ = QVector2D(e->localPos());

  if (e->buttons() & Qt::LeftButton) {
    // Rotation
    QVector3D n = QVector3D(diff.y(), diff.x(), 0.0).normalized();
    float angle = diff.length() / ROTATION_SENSITIVITY;
    rotation_ = QQuaternion::fromAxisAndAngle(n, angle) * rotation_;
  } else if (e->buttons() & Qt::RightButton) {
    // Pan
    float scale = distance_ * PAN_SENSITIVITY;
    pan_x_ += diff.x() * scale;
    pan_y_ -= diff.y() * scale;
  }
  update();
}

void Chiplet3DWidget::wheelEvent(QWheelEvent* e)
{
  if (e->angleDelta().y() > 0) {
    distance_ *= ZOOM_IN_FACTOR;
  } else {
    distance_ *= ZOOM_OUT_FACTOR;
  }
  update();
}

}  // namespace gui
