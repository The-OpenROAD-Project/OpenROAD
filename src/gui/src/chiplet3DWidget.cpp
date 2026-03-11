// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include "chiplet3DWidget.h"

#include <QDateTime>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "gui/gui.h"
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
constexpr float kMinDistanceBound = 50.0f;
constexpr float kMaxDistanceBound = 1e7f;
constexpr float kZScale = 2.0f;

// Rendering constants
const QColor kBackgroundColor(26, 26, 26);
const QColor kGridColor(76, 76, 76);
constexpr int kLine3DWidth = 2;
constexpr float kHighlightZOffset = 2.0f;
constexpr int kTopFaceLightness = 110;
constexpr int kBottomFaceDarkness = 110;
constexpr int kSideFaceDarkness = 105;

static const std::array<QVector3D, 9> kColorPalette = {{
    {0.0f, 1.0f, 0.0f},  // Green
    {0.0f, 0.5f, 0.5f},  // Teal
    {0.0f, 1.0f, 1.0f},  // Cyan
    {1.0f, 0.0f, 1.0f},  // Magenta
    {1.0f, 0.5f, 0.0f},  // Orange
    {0.5f, 0.5f, 1.0f},  // Blue-ish
    {1.0f, 0.0f, 0.0f},  // Red
    {0.5f, 0.0f, 0.5f},  // Purple
    {1.0f, 0.5f, 0.5f}   // Pink
}};
}  // namespace

namespace gui {

Chiplet3DWidget::Chiplet3DWidget(const SelectionSet& selected,
                                 const HighlightSet& highlighted,
                                 QWidget* parent)
    : QWidget(parent), selected_(selected), highlighted_(highlighted)
{
  // Optimize for painting speed
  setAttribute(Qt::WA_OpaquePaintEvent);
}

void Chiplet3DWidget::setChip(odb::dbChip* chip)
{
  chip_ = chip;
  pan_x_ = 0.0f;
  pan_y_ = 0.0f;
  rotation_ = QQuaternion();
  buildGeometries();
  update();
}

void Chiplet3DWidget::setLogger(utl::Logger* logger)
{
  logger_ = logger;
}

void Chiplet3DWidget::zoomTo(const Selected& selection)
{
  if (!chip_ || !selection) {
    return;
  }

  odb::Rect bbox;
  if (!selection.getBBox(bbox)) {
    return;
  }

  const odb::Cuboid global_cuboid = chip_->getCuboid();

  float target_x = bbox.xCenter() - global_cuboid.xCenter();
  float target_y = bbox.yCenter() - global_cuboid.yCenter();

  // Set pan to negative of target because we translate the scene
  // TODO: Normalize pan calculations by dbUnitsPerMicron to prevent scale
  // issues on extreme layouts
  pan_x_ = -target_x;
  pan_y_ = -target_y;

  // Reset rotation to look directly from above (2D-like view)
  rotation_ = QQuaternion();

  // Set distance based on rect size to zoom in
  float max_dim = std::max(bbox.dx(), bbox.dy());

  float min_distance = 100.0f;
  if (chip_->getBlock()) {
    min_distance = chip_->getBlock()->getDbUnitsPerMicron() * 10.0f;
  }

  distance_ = std::max(max_dim * 1.5f, min_distance);

  update();
}

void Chiplet3DWidget::selectionFocus(const Selected& focus)
{
  if (animate_selection_ != nullptr) {
    animate_selection_->timer->stop();
    animate_selection_ = nullptr;
  }

  if (focus) {
    const int update_interval = 100;
    const int state_reset_interval = 3;
    animate_selection_ = std::make_unique<AnimatedSelected>(AnimatedSelected{
        .selection = focus,
        .state_count = 0,
        .max_state_count = state_reset_interval * kAnimationRepeats,
        .state_modulo = state_reset_interval,
        .timer = nullptr});
    animate_selection_->timer = std::make_unique<QTimer>();
    animate_selection_->timer->setInterval(update_interval);

    const qint64 max_animate_time = QDateTime::currentMSecsSinceEpoch()
                                    + (animate_selection_->max_state_count + 2)
                                          * (qint64) update_interval;
    connect(
        animate_selection_->timer.get(),
        &QTimer::timeout,
        this,
        [this, max_animate_time]() {
          if (animate_selection_ == nullptr) {
            return;
          }

          animate_selection_->state_count++;
          if (animate_selection_->max_state_count != 0
              && (animate_selection_->state_count
                      == animate_selection_->max_state_count
                  || QDateTime::currentMSecsSinceEpoch() > max_animate_time)) {
            animate_selection_->timer->stop();
            animate_selection_ = nullptr;
          }
          update();
        });

    animate_selection_->timer->start();
  }
}

void Chiplet3DWidget::buildGeometries()
{
  if (!chip_) {
    return;
  }
  const odb::Cuboid global_cuboid = chip_->getCuboid();
  const odb::UnfoldedModel model(logger_, chip_);
  center_transform_ = odb::dbTransform(odb::Point3D(-global_cuboid.xCenter(),
                                                    -global_cuboid.yCenter(),
                                                    -global_cuboid.zCenter()));

  vertices_.clear();
  faces_.clear();
  chip_cuboids_.clear();

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
    chip_cuboids_.push_back(chip.cuboid);
    odb::Cuboid draw_cuboid = chip.cuboid;
    center_transform_.apply(draw_cuboid);
    // Color by Depth (proportional to Z)
    const QVector3D color = kColorPalette[index++ % kColorPalette.size()];

    const uint32_t base = vertices_.size();
    for (const auto& p : draw_cuboid.getPoints()) {
      vertices_.push_back({QVector3D(p.x(), p.y(), p.z() * kZScale), color});
    }

    QColor baseColor;
    baseColor.setRgbF(color.x(), color.y(), color.z());

    odb::dbBlock* block = nullptr;
    if (!chip.chip_inst_path.empty()) {
      if (auto* master = chip.chip_inst_path.back()->getMasterChip()) {
        block = master->getBlock();
      }
    } else if (chip_) {
      block = chip_->getBlock();
    }

    Selected sel;
    if (block) {
      sel = Gui::get()->makeSelected(block);
    }

    faces_.push_back({{base + 0, base + 3, base + 2, base + 1},
                      baseColor.darker(kBottomFaceDarkness),
                      sel});  // Bottom
    faces_.push_back({{base + 4, base + 5, base + 6, base + 7},
                      baseColor.lighter(kTopFaceLightness),
                      sel});  // Top
    faces_.push_back({{base + 0, base + 1, base + 5, base + 4},
                      baseColor.darker(kSideFaceDarkness),
                      sel});  // Front
    faces_.push_back(
        {{base + 1, base + 2, base + 6, base + 5}, baseColor, sel});  // Right
    faces_.push_back({{base + 2, base + 3, base + 7, base + 6},
                      baseColor.darker(kSideFaceDarkness),
                      sel});  // Back
    faces_.push_back(
        {{base + 0, base + 4, base + 7, base + 3}, baseColor, sel});  // Left
  }
}

void Chiplet3DWidget::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // 1. Clear Background
  painter.fillRect(rect(), kBackgroundColor);

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
  painter.setPen(QPen(kGridColor, 1));
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
               kGridColor,
               modelView,
               projection,
               rect(),
               zNear);
    drawLine3D(painter,
               QVector3D(-grid_size, pos, grid_z),
               QVector3D(grid_size, pos, grid_z),
               kGridColor,
               modelView,
               projection,
               rect(),
               zNear);
  }

  // 4. Draw Chiplets
  if (vertices_.empty()) {
    return;
  }

  sorted_faces_.clear();
  sorted_faces_.reserve(faces_.size());

  for (const auto& face : faces_) {
    QVector3D v0 = modelView * vertices_[face.indices[0]].position;
    QVector3D v1 = modelView * vertices_[face.indices[1]].position;
    QVector3D v2 = modelView * vertices_[face.indices[2]].position;

    // Back-face culling: Normal in view space
    QVector3D normal = QVector3D::crossProduct(v1 - v0, v2 - v0);
    // In perspective projection, the view vector from eye to vertex is just v0
    // A face is visible if it points towards the camera: dot(normal,
    // view_vector) < 0
    if (QVector3D::dotProduct(normal, v0) >= 0.0f) {
      continue;
    }

    QVector3D v3 = modelView * vertices_[face.indices[3]].position;
    float minZ = std::min({v0.z(), v1.z(), v2.z(), v3.z()});

    sorted_faces_.push_back({&face, minZ});
  }

  std::ranges::sort(sorted_faces_,
                    [](const ProjectedFace& a, const ProjectedFace& b) {
                      return a.sortZ < b.sortZ;
                    });

  for (const auto& pf : sorted_faces_) {
    drawFace3D(painter, *(pf.face), modelView, projection, rect(), zNear);
  }

  if (animate_selection_ != nullptr) {
    bool draw_highlight = true;
    if (animate_selection_->state_count % animate_selection_->state_modulo
        == 0) {
      draw_highlight = false;
    }
    odb::Rect bbox;
    if (draw_highlight && animate_selection_->selection.getBBox(bbox)) {
      float target_z = 0.0f;
      if (chip_) {
        for (const auto& cuboid : chip_cuboids_) {
          odb::Rect c_rect(
              cuboid.xMin(), cuboid.yMin(), cuboid.xMax(), cuboid.yMax());
          if (c_rect.intersects(bbox)) {
            odb::Cuboid draw_cuboid = cuboid;
            center_transform_.apply(draw_cuboid);
            target_z = draw_cuboid.zMax() * kZScale + kHighlightZOffset;
            break;
          }
        }

        odb::Point pt_min(bbox.xMin(), bbox.yMin());
        odb::Point pt_max(bbox.xMax(), bbox.yMax());
        center_transform_.apply(pt_min);
        center_transform_.apply(pt_max);

        QVector3D p1(pt_min.x(), pt_min.y(), target_z);
        QVector3D p2(pt_max.x(), pt_min.y(), target_z);
        QVector3D p3(pt_max.x(), pt_max.y(), target_z);
        QVector3D p4(pt_min.x(), pt_max.y(), target_z);

        QColor highlight_color(Painter::kHighlight.r,
                               Painter::kHighlight.g,
                               Painter::kHighlight.b,
                               200);

        drawLine3D(painter,
                   p1,
                   p2,
                   highlight_color,
                   modelView,
                   projection,
                   rect(),
                   zNear);
        drawLine3D(painter,
                   p2,
                   p3,
                   highlight_color,
                   modelView,
                   projection,
                   rect(),
                   zNear);
        drawLine3D(painter,
                   p3,
                   p4,
                   highlight_color,
                   modelView,
                   projection,
                   rect(),
                   zNear);
        drawLine3D(painter,
                   p4,
                   p1,
                   highlight_color,
                   modelView,
                   projection,
                   rect(),
                   zNear);
      }
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
                                 const QRect& viewport,
                                 float zNear)
{
  QVector3D p1_view = modelView * p1_world;
  QVector3D p2_view = modelView * p2_world;

  const float kClipZ = -zNear;

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

  painter.setPen(QPen(color, kLine3DWidth));
  painter.drawLine(s1, s2);
}

void Chiplet3DWidget::drawFace3D(QPainter& painter,
                                 const Face& face,
                                 const QMatrix4x4& modelView,
                                 const QMatrix4x4& projection,
                                 const QRect& viewport,
                                 float zNear)
{
  const float kClipZ = -zNear;

  face_view_points_.clear();
  for (int i = 0; i < 4; ++i) {
    face_view_points_.push_back(modelView
                                * vertices_[face.indices[i]].position);
  }

  face_clipped_points_.clear();
  for (size_t i = 0; i < face_view_points_.size(); ++i) {
    const QVector3D& p1 = face_view_points_[i];
    const QVector3D& p2 = face_view_points_[(i + 1) % face_view_points_.size()];

    bool p1_visible = p1.z() < kClipZ;
    bool p2_visible = p2.z() < kClipZ;

    if (p1_visible) {
      face_clipped_points_.push_back(p1);
    }

    if (p1_visible != p2_visible) {
      float t = (kClipZ - p1.z()) / (p2.z() - p1.z());
      QVector3D intersection = p1 + (p2 - p1) * t;
      face_clipped_points_.push_back(intersection);
    }
  }

  if (face_clipped_points_.empty()) {
    return;
  }

  const float w = viewport.width();
  const float h = viewport.height();

  polygon_buffer_.clear();
  for (const auto& p_view : face_clipped_points_) {
    QVector3D p_ndc = projection.map(p_view);
    polygon_buffer_ << QPointF((p_ndc.x() + 1.0f) * 0.5f * w,
                               (1.0f - p_ndc.y()) * 0.5f * h);
  }

  QColor color = face.color;
  int pen_width = 1;

  if (face.selection) {
    int highlight_group = 0;
    bool is_highlighted = false;
    for (const auto& h_set : highlighted_) {
      if (h_set.contains(face.selection)) {
        is_highlighted = true;
        break;
      }
      highlight_group++;
    }

    if (is_highlighted) {
      Painter::Color hc
          = Painter::kHighlightColors[highlight_group % gui::kNumHighlightSet];
      color = QColor(hc.r, hc.g, hc.b, 200);
      pen_width = 3;
    } else if (selected_.contains(face.selection)) {
      color = QColor(Painter::kHighlight.r,
                     Painter::kHighlight.g,
                     Painter::kHighlight.b,
                     200);
      pen_width = 3;
    }

    bool matches_anim = false;
    if (animate_selection_ != nullptr) {
      if (animate_selection_->selection == face.selection) {
        matches_anim = true;
      }
    }

    if (matches_anim) {
      int state
          = animate_selection_->state_count % animate_selection_->state_modulo;
      pen_width = state + 2;
      color = QColor(Painter::kHighlight.r,
                     Painter::kHighlight.g,
                     Painter::kHighlight.b,
                     200);
    }
  }

  painter.setBrush(QBrush(color));
  painter.setPen(QPen(color.darker(), pen_width));
  painter.drawPolygon(polygon_buffer_);
}

void Chiplet3DWidget::mousePressEvent(QMouseEvent* e)
{
  mouse_press_position_ = QVector2D(e->localPos());
}

void Chiplet3DWidget::mouseMoveEvent(QMouseEvent* e)
{
  const QVector2D diff = QVector2D(e->localPos()) - mouse_press_position_;
  if (diff.lengthSquared() < 1e-6f) {
    return;
  }
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
  distance_ = std::clamp(distance_, kMinDistanceBound, kMaxDistanceBound);
  update();
}

}  // namespace gui
