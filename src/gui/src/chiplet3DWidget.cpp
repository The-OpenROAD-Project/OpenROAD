// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include "chiplet3DWidget.h"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"
#include "odb/unfoldedModel.h"
#include "utl/Logger.h"

namespace {
constexpr float kInitialDistanceFactor = 3.0f;
constexpr float kLayerGapFactor = 2.0f;
constexpr float kMinDistanceCheck = 100.0f;
constexpr float kDefaultDistance = 1000.0f;
constexpr float kDefaultSafeSize = 1000.0f;
constexpr float kNearFarFactor = 2.0f;
constexpr float kMinZNear = 10.0f;
constexpr float kMaxZFarOffset = 10000.0f;
constexpr float kGridSizeFactor = 1.5f;
constexpr float kGridSteps = 5.0f;
constexpr float kGridZOffsetFactor = 0.05f;
constexpr int kGridLineCount = 5;
constexpr float kRotationSensitivity = 2.0f;
constexpr float kPanSensitivity = 0.002f;
constexpr float kZoomInFactor = 0.9f;
constexpr float kZoomOutFactor = 1.1f;

static const std::array<QVector3D, 7> kColorPalette = {{
    {0.0f, 1.0f, 0.0f},  // Green
    {1.0f, 1.0f, 0.0f},  // Yellow
    {0.0f, 1.0f, 1.0f},  // Cyan
    {1.0f, 0.0f, 1.0f},  // Magenta
    {1.0f, 0.5f, 0.0f},  // Orange
    {0.5f, 0.5f, 1.0f},  // Blue-ish
    {1.0f, 0.0f, 0.0f}   // Red
}};
}  // namespace

namespace gui {

Chiplet3DWidget::Chiplet3DWidget(QWidget* parent) : QWidget(parent)
{
  // Optimize for painting speed
  setAttribute(Qt::WA_OpaquePaintEvent);
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

void Chiplet3DWidget::buildGeometries()
{
  if (!chip_) {
    return;
  }
  const odb::Cuboid global_cuboid = chip_->getCuboid();
  const odb::UnfoldedModel model(logger_, chip_);
  const odb::dbTransform center_transform
      = odb::dbTransform(odb::Point3D(-global_cuboid.xCenter(),
                                      -global_cuboid.yCenter(),
                                      -global_cuboid.zCenter()));

  vertices_.clear();
  indices_lines_.clear();

  // Center and Camera calculations
  const float cx = (global_cuboid.xMin() + global_cuboid.xMax()) / 2.0f;
  const float cy = (global_cuboid.yMin() + global_cuboid.yMax()) / 2.0f;
  center_ = QVector3D(cx, cy, 0.0f);

  const float dx = global_cuboid.dx();
  const float dy = global_cuboid.dy();
  const float dz = global_cuboid.dz() * kLayerGapFactor;
  bounding_radius_ = std::sqrt(dx * dx + dy * dy + dz * dz) / 2.0f;

  distance_ = bounding_radius_ * kInitialDistanceFactor;
  if (distance_ < kMinDistanceCheck) {
    distance_ = kDefaultDistance;
  }

  int index = 0;
  for (const auto& chip : model.getChips()) {
    odb::Cuboid draw_cuboid = chip.cuboid;
    center_transform.apply(draw_cuboid);
    // Color by Depth (proportional to Z)
    const QVector3D color = kColorPalette[index++ % kColorPalette.size()];

    const uint32_t base = vertices_.size();
    for (const auto& p : draw_cuboid.getPoints()) {
      vertices_.push_back({QVector3D(p.x(), p.y(), p.z()), color});
    }

    // Add line indices for a cube (12 lines)
    const uint32_t lines[24] = {0, 1, 1, 2, 2, 3, 3, 0,   // Bottom face
                                4, 5, 5, 6, 6, 7, 7, 4,   // Top face
                                0, 4, 1, 5, 2, 6, 3, 7};  // Connecting pillars

    for (const uint32_t i : lines) {
      indices_lines_.push_back(base + i);
    }
  }
}

void Chiplet3DWidget::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // 1. Clear Background
  painter.fillRect(rect(), QColor(26, 26, 26));  // Approx 0.1f grey

  // 2. Setup Camera Matrices

  // Use bounding_radius_ which is rotation-invariant
  float safe_size = bounding_radius_;
  if (safe_size < 1.0f) {
    safe_size = kDefaultSafeSize;
  }

  const float zNear
      = std::max(distance_ - safe_size * kNearFarFactor, kMinZNear);
  float zFar = distance_ + safe_size * kNearFarFactor;

  if (zFar < zNear + kMinDistanceCheck) {
    zFar = zNear + kMaxZFarOffset;
  }

  // Projection Matrix
  const qreal aspect = qreal(width()) / qreal(height() ? height() : 1);
  QMatrix4x4 projection;
  projection.perspective(45.0f, aspect, zNear, zFar);

  // ModelView Matrix
  QMatrix4x4 modelView;
  modelView.translate(pan_x_, pan_y_, -distance_);
  modelView.rotate(rotation_);

  // 3. Draw Grid
  painter.setPen(QPen(QColor(76, 76, 76), 1));  // 0.3f grey
  const float grid_size
      = safe_size * kGridSizeFactor;  // Adjust grid to scene size
  const float step = grid_size / kGridSteps;
  const float grid_z = -center_.z() - (distance_ * kGridZOffsetFactor * 0.5f);

  for (int i = -kGridLineCount; i <= kGridLineCount; ++i) {
    const float pos = i * step;
    // Lines parallel to Z (or Y in local, depending on orientation)
    // The original code drew grid on Z plane.
    drawLine3D(painter,
               QVector3D(pos, -grid_size, grid_z),
               QVector3D(pos, grid_size, grid_z),
               QColor(76, 76, 76),
               modelView,
               projection,
               rect());
    drawLine3D(painter,
               QVector3D(-grid_size, pos, grid_z),
               QVector3D(grid_size, pos, grid_z),
               QColor(76, 76, 76),
               modelView,
               projection,
               rect());
  }

  // 4. Draw Chiplets
  if (vertices_.empty()) {
    return;
  }

  // Process lines
  for (size_t i = 0; i < indices_lines_.size(); i += 2) {
    const uint32_t idx1 = indices_lines_[i];
    const uint32_t idx2 = indices_lines_[i + 1];

    if (idx1 < vertices_.size() && idx2 < vertices_.size()) {
      const VertexData& v1 = vertices_[idx1];
      const VertexData& v2 = vertices_[idx2];

      QColor c;
      c.setRgbF(v1.color.x(), v1.color.y(), v1.color.z());

      // Draw using helper
      drawLine3D(
          painter, v1.position, v2.position, c, modelView, projection, rect());
    }
  }
}

// Helper: Projects 3D points to 2D, Handles Z-Clipping
void Chiplet3DWidget::drawLine3D(QPainter& painter,
                                 const QVector3D& p1_world,
                                 const QVector3D& p2_world,
                                 const QColor& color,
                                 const QMatrix4x4& modelView,
                                 const QMatrix4x4& projection,
                                 const QRect& viewport)
{
  // 1. Transform to View Space
  QVector3D p1_view = modelView * p1_world;
  QVector3D p2_view = modelView * p2_world;

  // 2. Clip against Near Plane
  // In OpenGL view space, camera looks down -Z.
  // Near plane is at z = -nearVal (e.g. -10.0). Points with z > -nearVal are
  // behind the near plane. However, QMatrix4x4::perspective sets up a standard
  // frustum. We can extract the near plane distance from our setup, but a small
  // fixed epsilon usually works for preventing divide-by-zero or "behind head"
  // artifacts.

  // Simple clipping: Clip against Z = -0.1 (very close to camera)
  const float kClipZ = -0.1f;

  const bool p1_visible = p1_view.z() < kClipZ;
  const bool p2_visible = p2_view.z() < kClipZ;

  if (!p1_visible && !p2_visible) {
    return;  // Both behind camera
  }

  if (p1_visible != p2_visible) {
    // Line spans the clip plane. Calculate intersection.
    // t = (kClipZ - z1) / (z2 - z1)
    const float t = (kClipZ - p1_view.z()) / (p2_view.z() - p1_view.z());
    const QVector3D intersection = p1_view + (p2_view - p1_view) * t;

    if (!p1_visible) {
      p1_view = intersection;
    } else {
      p2_view = intersection;
    }
  }

  // NDC or Normalized Device Coordinates
  const QVector3D p1_ndc = projection.map(p1_view);
  const QVector3D p2_ndc = projection.map(p2_view);

  // Map NDC (-1 to 1) to Viewport (0 to width/height)
  const float w = viewport.width();
  const float h = viewport.height();

  // Note: NDC Y is up, Screen Y is down.
  const QPointF s1((p1_ndc.x() + 1.0f) * 0.5f * w,
                   (1.0f - p1_ndc.y()) * 0.5f * h);
  const QPointF s2((p2_ndc.x() + 1.0f) * 0.5f * w,
                   (1.0f - p2_ndc.y()) * 0.5f * h);

  // 4. Draw
  painter.setPen(QPen(color, 2));
  painter.drawLine(s1, s2);
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
  const QVector2D diff = QVector2D(e->localPos()) - mouse_press_position_;
  mouse_press_position_ = QVector2D(e->localPos());

  if (e->buttons() & Qt::LeftButton) {
    // Rotation
    const QVector3D n = QVector3D(diff.y(), diff.x(), 0.0).normalized();
    const float angle = diff.length() / kRotationSensitivity;
    rotation_ = QQuaternion::fromAxisAndAngle(n, angle) * rotation_;
  } else if (e->buttons() & Qt::RightButton) {
    // Pan
    const float scale = distance_ * kPanSensitivity;
    pan_x_ += diff.x() * scale;
    pan_y_ -= diff.y() * scale;
  }
  update();
}

void Chiplet3DWidget::wheelEvent(QWheelEvent* e)
{
  if (e->angleDelta().y() > 0) {
    distance_ *= kZoomInFactor;
  } else {
    distance_ *= kZoomOutFactor;
  }
  update();
}

}  // namespace gui
