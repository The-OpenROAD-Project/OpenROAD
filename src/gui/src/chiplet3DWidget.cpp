// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include "chiplet3DWidget.h"

#include <QDateTime>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <algorithm>
#include <any>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <queue>
#include <ranges>
#include <utility>
#include <variant>
#include <vector>

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
static_assert(
    kRotationSensitivity > 0.0f,
    "kRotationSensitivity must be strictly positive to avoid division by zero");
constexpr float kPanSensitivity = 0.002f;
constexpr float kZoomInFactor = 0.9f;
constexpr float kZoomOutFactor = 1.1f;
constexpr float kMinDistanceFraction = 0.1f;
constexpr float kMaxDistanceFraction = 20.0f;
constexpr float kZScale = 2.0f;
constexpr float kZoomFactorDefault = 2.2f;
constexpr float kZoomFactorExpanded = 2.5f;
constexpr float kCoordinateTolerance = 1e-3f;
constexpr double kPolygonOffsetChiplet = 1e-5;
constexpr double kPolygonOffsetFace = 1e-6;
constexpr double kDepthTolerance = 1e-5;
constexpr double kSortZTolerance = 1e-6;
constexpr float kMouseClickTolerance = 1e-6f;

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
  setAttribute(Qt::WA_OpaquePaintEvent);
}

Chiplet3DWidget::~Chiplet3DWidget()
{
  if (animate_selection_ != nullptr) {
    animate_selection_->timer->stop();
    animate_selection_ = nullptr;
  }
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
  float target_z = 0.0f;

  float max_dim = std::max(bbox.dx(), bbox.dy());
  float zoom_factor = kZoomFactorDefault;

  if (selection.getTypeName() == "Marker") {
    auto marker_ptr = std::any_cast<odb::dbMarker*>(&selection.getObject());
    if (marker_ptr && *marker_ptr) {
      auto marker = *marker_ptr;
      bool has_cuboid = false;
      int min_x = std::numeric_limits<int>::max();
      int min_y = std::numeric_limits<int>::max();
      int min_z = std::numeric_limits<int>::max();
      int max_x = std::numeric_limits<int>::lowest();
      int max_y = std::numeric_limits<int>::lowest();
      int max_z = std::numeric_limits<int>::lowest();

      for (const auto& shape : marker->getShapes()) {
        if (std::holds_alternative<odb::Cuboid>(shape)) {
          const odb::Cuboid cuboid = std::get<odb::Cuboid>(shape);
          min_x = std::min(min_x, cuboid.xMin());
          min_y = std::min(min_y, cuboid.yMin());
          min_z = std::min(min_z, cuboid.zMin());
          max_x = std::max(max_x, cuboid.xMax());
          max_y = std::max(max_y, cuboid.yMax());
          max_z = std::max(max_z, cuboid.zMax());
          has_cuboid = true;
        }
      }

      if (has_cuboid) {
        float cx = (min_x + max_x) / 2.0f;
        float cy = (min_y + max_y) / 2.0f;
        float cz = (min_z + max_z) / 2.0f;

        target_x = cx - global_cuboid.xCenter();
        target_y = cy - global_cuboid.yCenter();
        target_z = (cz - global_cuboid.zMin()) * kZScale;

        float dx = max_x - min_x;
        float dy = max_y - min_y;
        float dz = (max_z - min_z) * kZScale;

        max_dim = std::max({dx, dy, dz});
        zoom_factor = kZoomFactorExpanded;
      }
    }
  }

  QQuaternion pitch = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), -45.0f);
  QQuaternion yaw = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), 45.0f);
  rotation_ = pitch * yaw;

  QVector3D p_drawn(target_x, target_y, target_z);
  QVector3D p_rot = rotation_ * p_drawn;

  pan_x_ = -p_rot.x();
  pan_y_ = -p_rot.y();

  float min_distance = 100.0f;
  if (auto* block = chip_->getBlock()) {
    min_distance = block->getDbUnitsPerMicron() * 10.0f;
  }

  distance_ = std::max(max_dim * zoom_factor, min_distance);

  const float min_dist_limit
      = std::max(bounding_radius_ * kMinDistanceFraction, 1.0f);
  const float max_dist_limit
      = std::max(bounding_radius_ * kMaxDistanceFraction, min_dist_limit);
  distance_ = std::clamp(distance_, min_dist_limit, max_dist_limit);

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
    animate_selection_->timer = std::make_unique<QTimer>(this);
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
                                                    -global_cuboid.zMin()));

  vertices_.clear();
  faces_.clear();
  chip_cuboids_.clear();
  chiplet_objects_.clear();

  // Camera calculations
  const float dx = global_cuboid.dx();
  const float dy = global_cuboid.dy();
  const float dz = global_cuboid.dz() * kLayerGapFactor;
  bounding_radius_ = std::sqrt(dx * dx + dy * dy + dz * dz) / 2.0f;

  distance_ = bounding_radius_ * kInitialDistanceFactor;
  if (distance_ < kMinDistanceCheck) {
    distance_ = kDefaultDistance;
  }

  int color_index = 0;
  const auto& chips = model.getChips();

  for (size_t chip_idx = 0; chip_idx < chips.size(); ++chip_idx) {
    const auto& chip = chips[chip_idx];
    chip_cuboids_.push_back(chip.cuboid);
    odb::Cuboid draw_cuboid = chip.cuboid;
    center_transform_.apply(draw_cuboid);

    const QVector3D color = kColorPalette[color_index++ % kColorPalette.size()];

    const uint32_t base = vertices_.size();
    for (const auto& p : draw_cuboid.getPoints()) {
      vertices_.push_back({QVector3D(p.x(), p.y(), p.z() * kZScale)});
    }

    ChipletObject chiplet_obj;
    chiplet_obj.world_z_min = draw_cuboid.zMin() * kZScale;
    chiplet_obj.world_z_max = draw_cuboid.zMax() * kZScale;
    chiplet_obj.world_x_min = draw_cuboid.xMin();
    chiplet_obj.world_x_max = draw_cuboid.xMax();
    chiplet_obj.world_y_min = draw_cuboid.yMin();
    chiplet_obj.world_y_max = draw_cuboid.yMax();

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

    const int chiplet_idx = static_cast<int>(chip_idx);
    const int face_base = static_cast<int>(faces_.size());

    // IMPORTANT: Winding order must produce outward-pointing normals
    // via cross(v1-v0, v2-v0) for correct back-face culling.
    //
    // Assuming getPoints() returns:
    //   0=(xmin,ymin,zmin), 1=(xmax,ymin,zmin),
    //   2=(xmax,ymax,zmin), 3=(xmin,ymax,zmin),
    //   4=(xmin,ymin,zmax), 5=(xmax,ymin,zmax),
    //   6=(xmax,ymax,zmax), 7=(xmin,ymax,zmax)
    //
    // Bottom normal = -Z (outward=down):  {0, 3, 2, 1}
    // Top    normal = +Z (outward=up):    {4, 5, 6, 7}  <-- NOT {4,7,6,5}
    // Front  normal = -Y (outward=front): {0, 1, 5, 4}
    // Right  normal = +X (outward=right): {1, 2, 6, 5}
    // Back   normal = +Y (outward=back):  {2, 3, 7, 6}
    // Left   normal = -X (outward=left):  {0, 4, 7, 3}

    faces_.push_back({{base + 0, base + 3, base + 2, base + 1},
                      baseColor.darker(kBottomFaceDarkness),
                      sel,
                      chiplet_idx});  // Bottom
    faces_.push_back({{base + 4, base + 5, base + 6, base + 7},
                      baseColor.lighter(kTopFaceLightness),
                      sel,
                      chiplet_idx});  // Top
    faces_.push_back({{base + 0, base + 1, base + 5, base + 4},
                      baseColor.darker(kSideFaceDarkness),
                      sel,
                      chiplet_idx});  // Front
    faces_.push_back({{base + 1, base + 2, base + 6, base + 5},
                      baseColor,
                      sel,
                      chiplet_idx});  // Right
    faces_.push_back({{base + 2, base + 3, base + 7, base + 6},
                      baseColor.darker(kSideFaceDarkness),
                      sel,
                      chiplet_idx});  // Back
    faces_.push_back({{base + 0, base + 4, base + 7, base + 3},
                      baseColor,
                      sel,
                      chiplet_idx});  // Left

    for (int fi = face_base; fi < face_base + 6; ++fi) {
      chiplet_obj.face_indices.push_back(fi);
    }

    chiplet_objects_.push_back(std::move(chiplet_obj));
  }
}

void Chiplet3DWidget::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // 1. Clear Background
  painter.fillRect(rect(), kBackgroundColor);

  // 2. Setup Camera Matrices
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

  const qreal aspect = qreal(width()) / qreal(height() ? height() : 1);
  QMatrix4x4 projection;
  projection.perspective(45.0f, aspect, zNear, zFar);

  QMatrix4x4 modelView;
  modelView.translate(pan_x_, pan_y_, -distance_);
  modelView.rotate(rotation_);

  // 3. Draw Grid
  painter.setPen(QPen(kGridColor, 1));
  const float grid_size = safe_size * kGridSizeFactor;
  const float step = grid_size / kGridSteps;
  const float grid_z = -(bounding_radius_ * kGridZOffsetFactor);

  for (int i = -kGridLineCount; i <= kGridLineCount; ++i) {
    const float pos = i * step;
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

  // 4. Draw Chiplets with back-face culling + depth sorting
  if (vertices_.empty() || chiplet_objects_.empty()) {
    return;
  }

  // Step 4a: Project all faces, cull back-facing ones.
  // We determine the outward normal geometrically (from chiplet centroid
  // toward face center) so we don't depend on vertex winding order.
  sorted_faces_.clear();
  sorted_faces_.reserve(faces_.size());

  for (const auto& face : faces_) {
    QVector3D v0 = modelView * vertices_[face.indices[0]].position;
    QVector3D v1 = modelView * vertices_[face.indices[1]].position;
    QVector3D v2 = modelView * vertices_[face.indices[2]].position;
    QVector3D v3 = modelView * vertices_[face.indices[3]].position;

    QVector3D face_center = (v0 + v1 + v2 + v3) * 0.25f;

    // The winding order is designed to produce an outward-pointing normal.
    QVector3D cross_normal = QVector3D::crossProduct(v1 - v0, v2 - v0);

    bool is_back_facing
        = QVector3D::dotProduct(cross_normal, face_center) >= 0.0f;

    if (is_back_facing) {
      continue;
    }

    double avgZ = (v0.z() + v1.z() + v2.z() + v3.z()) * 0.25;

    double face_polygon_offset
        = static_cast<double>(face.chiplet_index) * kPolygonOffsetChiplet
          + static_cast<double>(&face - faces_.data()) * kPolygonOffsetFace;

    sorted_faces_.push_back({&face, avgZ + face_polygon_offset});
  }

  std::vector<double> chiplet_view_depths(chiplet_objects_.size(), 0.0);
  for (size_t i = 0; i < chiplet_objects_.size(); ++i) {
    double max_z = -1e9;
    for (int face_idx : chiplet_objects_[i].face_indices) {
      for (uint32_t v_idx : faces_[face_idx].indices) {
        QVector3D view_p = modelView * vertices_[v_idx].position;
        max_z = std::max(static_cast<double>(view_p.z()), max_z);
      }
    }

    // Emulation of glPolygonOffset to break the Z coplanarity tie of chiplets
    double polygon_offset = static_cast<double>(i) * kPolygonOffsetChiplet;
    chiplet_view_depths[i] = max_z + polygon_offset;
  }

  // Step 4b: Sort front-facing faces by view-space depth (farthest first)
  // To avoid Painter's algorithm artifacts between different chiplets and
  // prevent std::stable_sort Undefined Behavior (Strict Weak Ordering
  // violation), we primarily sort by the chiplet's centroid depth.

  // We calculate the viewing direction in world space. Since modelView is
  // constructed by translating and then rotating (using the rotation_
  // quaternion), the viewing direction can be robustly computed by rotating the
  // default view vector (0, 0, -1) by the inverse (conjugate) of the rotation
  // quaternion.
  QVector3D view_dir_world
      = rotation_.conjugated().rotatedVector(QVector3D(0, 0, -1));
  bool looking_down = view_dir_world.z() <= 0.001f;
  bool look_neg_x = view_dir_world.x() <= 0.0f;
  bool look_neg_y = view_dir_world.y() <= 0.0f;

  auto overlap1D = [&](double min1, double max1, double min2, double max2) {
    return (min1 <= max2 + kCoordinateTolerance
            && max1 >= min2 - kCoordinateTolerance);
  };

  auto overlapX = [&](const auto& a, const auto& b) {
    return overlap1D(
        a.world_x_min, a.world_x_max, b.world_x_min, b.world_x_max);
  };

  auto overlapY = [&](const auto& a, const auto& b) {
    return overlap1D(
        a.world_y_min, a.world_y_max, b.world_y_min, b.world_y_max);
  };

  auto overlapZ = [&](const auto& a, const auto& b) {
    return overlap1D(
        a.world_z_min, a.world_z_max, b.world_z_min, b.world_z_max);
  };

  size_t num_chiplets = chiplet_objects_.size();
  std::vector<std::vector<int>> adj(num_chiplets);
  std::vector<int> in_degree(num_chiplets, 0);

  for (size_t i = 0; i < num_chiplets; ++i) {
    for (size_t j = i + 1; j < num_chiplets; ++j) {
      const auto& chipA = chiplet_objects_[i];
      const auto& chipB = chiplet_objects_[j];

      bool overX = overlapX(chipA, chipB);
      bool overY = overlapY(chipA, chipB);
      bool overZ = overlapZ(chipA, chipB);

      if (chipA.world_z_max <= chipB.world_z_min + kCoordinateTolerance && overX
          && overY) {
        if (looking_down) {
          adj[i].push_back(j);
          in_degree[j]++;
        } else {
          adj[j].push_back(i);
          in_degree[i]++;
        }
      } else if (chipB.world_z_max <= chipA.world_z_min + kCoordinateTolerance
                 && overX && overY) {
        if (looking_down) {
          adj[j].push_back(i);
          in_degree[i]++;
        } else {
          adj[i].push_back(j);
          in_degree[j]++;
        }
      } else if (chipA.world_x_max <= chipB.world_x_min + kCoordinateTolerance
                 && overY && overZ) {
        if (look_neg_x) {
          adj[i].push_back(j);
          in_degree[j]++;
        } else {
          adj[j].push_back(i);
          in_degree[i]++;
        }
      } else if (chipB.world_x_max <= chipA.world_x_min + kCoordinateTolerance
                 && overY && overZ) {
        if (look_neg_x) {
          adj[j].push_back(i);
          in_degree[i]++;
        } else {
          adj[i].push_back(j);
          in_degree[j]++;
        }
      } else if (chipA.world_y_max <= chipB.world_y_min + kCoordinateTolerance
                 && overX && overZ) {
        if (look_neg_y) {
          adj[i].push_back(j);
          in_degree[j]++;
        } else {
          adj[j].push_back(i);
          in_degree[i]++;
        }
      } else if (chipB.world_y_max <= chipA.world_y_min + kCoordinateTolerance
                 && overX && overZ) {
        if (look_neg_y) {
          adj[j].push_back(i);
          in_degree[i]++;
        } else {
          adj[i].push_back(j);
          in_degree[j]++;
        }
      } else if (overX && overY && overZ) {
        bool a_contains_b
            = (chipA.world_x_min <= chipB.world_x_min + kCoordinateTolerance
               && chipA.world_x_max >= chipB.world_x_max - kCoordinateTolerance
               && chipA.world_y_min <= chipB.world_y_min + kCoordinateTolerance
               && chipA.world_y_max >= chipB.world_y_max - kCoordinateTolerance
               && chipA.world_z_min <= chipB.world_z_min + kCoordinateTolerance
               && chipA.world_z_max
                      >= chipB.world_z_max - kCoordinateTolerance);

        bool b_contains_a
            = (chipB.world_x_min <= chipA.world_x_min + kCoordinateTolerance
               && chipB.world_x_max >= chipA.world_x_max - kCoordinateTolerance
               && chipB.world_y_min <= chipA.world_y_min + kCoordinateTolerance
               && chipB.world_y_max >= chipA.world_y_max - kCoordinateTolerance
               && chipB.world_z_min <= chipA.world_z_min + kCoordinateTolerance
               && chipB.world_z_max
                      >= chipA.world_z_max - kCoordinateTolerance);

        if (a_contains_b && !b_contains_a) {
          if (looking_down) {
            adj[i].push_back(j);
            in_degree[j]++;
          } else {
            adj[j].push_back(i);
            in_degree[i]++;
          }
        } else if (b_contains_a && !a_contains_b) {
          if (looking_down) {
            adj[j].push_back(i);
            in_degree[i]++;
          } else {
            adj[i].push_back(j);
            in_degree[j]++;
          }
        }
      }
    }
  }

  // Comparator for priority queue to process furthest chiplets first (Painter's
  // algorithm). std::priority_queue puts elements with "lower" priority at the
  // bottom. By returning true when 'a' is closer than 'b', 'a' is given lower
  // priority. Thus, the furthest chiplets will appear at the top of the queue.
  auto is_closer = [&](int a, int b) {
    if (std::abs(chiplet_view_depths[a] - chiplet_view_depths[b])
        > kDepthTolerance) {
      return chiplet_view_depths[a] > chiplet_view_depths[b];
    }
    // Note: This index-based fallback is arbitrary and non-geometric.
    // If pathological Z-fighting visual artifacts appear during cycle
    // resolution, this fallback is the likely underlying cause.
    return a > b;
  };
  std::priority_queue<int, std::vector<int>, decltype(is_closer)> pq(is_closer);

  for (size_t i = 0; i < num_chiplets; ++i) {
    if (in_degree[i] == 0) {
      pq.push(i);
    }
  }

  std::vector<int> chiplet_order(num_chiplets, 0);
  int order_idx = 0;
  while (static_cast<size_t>(order_idx) < num_chiplets) {
    if (pq.empty()) {
      int best_node = -1;
      for (size_t i = 0; i < num_chiplets; ++i) {
        if (in_degree[i] > 0) {
          // Break cycles by forcibly picking the furthest available node.
          // If best_node is closer than i, update best_node to i.
          if (best_node == -1 || is_closer(best_node, i)) {
            best_node = static_cast<int>(i);
          }
        }
      }
      if (best_node != -1) {
        in_degree[best_node] = 0;
        pq.push(best_node);
      } else {
        break;
      }
    }

    int u = pq.top();
    pq.pop();
    chiplet_order[u] = order_idx++;
    for (int v : adj[u]) {
      if (in_degree[v] > 0) {
        if (--in_degree[v] == 0) {
          pq.push(v);
        }
      }
    }
  }

  std::ranges::stable_sort(
      sorted_faces_, [&](const ProjectedFace& a, const ProjectedFace& b) {
        if (a.face->chiplet_index != b.face->chiplet_index) {
          return chiplet_order[a.face->chiplet_index]
                 < chiplet_order[b.face->chiplet_index];
        }
        // Primary: face's view-space depth
        // (back-faces are already discarded during sorted_faces_
        // construction)
        if (std::abs(a.sortZ - b.sortZ) > kSortZTolerance) {
          return a.sortZ < b.sortZ;
        }
        return a.face < b.face;
      });

  // Step 4c: Draw all surviving front-facing faces back-to-front
  for (const auto& pf : sorted_faces_) {
    drawFace3D(painter, *(pf.face), modelView, projection, rect(), zNear);
  }

  // 5. Draw selection animation highlight
  if (animate_selection_ != nullptr) {
    bool draw_highlight = true;
    if (animate_selection_->state_count % animate_selection_->state_modulo
        == 0) {
      draw_highlight = false;
    }
    odb::Rect bbox;
    if (draw_highlight && animate_selection_->selection.getBBox(bbox)) {
      float target_z_max = 0.0f;
      float target_z_min = 0.0f;
      if (chip_) {
        for (const auto& cuboid : chip_cuboids_) {
          odb::Rect c_rect(
              cuboid.xMin(), cuboid.yMin(), cuboid.xMax(), cuboid.yMax());
          if (c_rect.intersects(bbox)) {
            odb::Cuboid draw_cuboid = cuboid;
            center_transform_.apply(draw_cuboid);
            target_z_max = draw_cuboid.zMax() * kZScale + kHighlightZOffset;
            target_z_min = draw_cuboid.zMin() * kZScale - kHighlightZOffset;
            break;
          }
        }

        odb::Point pt1(bbox.xMin(), bbox.yMin());
        odb::Point pt2(bbox.xMax(), bbox.yMin());
        odb::Point pt3(bbox.xMax(), bbox.yMax());
        odb::Point pt4(bbox.xMin(), bbox.yMax());

        center_transform_.apply(pt1);
        center_transform_.apply(pt2);
        center_transform_.apply(pt3);
        center_transform_.apply(pt4);

        QVector3D p1_top(pt1.x(), pt1.y(), target_z_max);
        QVector3D p2_top(pt2.x(), pt2.y(), target_z_max);
        QVector3D p3_top(pt3.x(), pt3.y(), target_z_max);
        QVector3D p4_top(pt4.x(), pt4.y(), target_z_max);

        QVector3D p1_bot(pt1.x(), pt1.y(), target_z_min);
        QVector3D p2_bot(pt2.x(), pt2.y(), target_z_min);
        QVector3D p3_bot(pt3.x(), pt3.y(), target_z_min);
        QVector3D p4_bot(pt4.x(), pt4.y(), target_z_min);

        QColor highlight_color(Painter::kHighlight.r,
                               Painter::kHighlight.g,
                               Painter::kHighlight.b,
                               255);

        // Top face
        drawLine3D(painter,
                   p1_top,
                   p2_top,
                   highlight_color,
                   modelView,
                   projection,
                   rect(),
                   zNear);
        drawLine3D(painter,
                   p2_top,
                   p3_top,
                   highlight_color,
                   modelView,
                   projection,
                   rect(),
                   zNear);
        drawLine3D(painter,
                   p3_top,
                   p4_top,
                   highlight_color,
                   modelView,
                   projection,
                   rect(),
                   zNear);
        drawLine3D(painter,
                   p4_top,
                   p1_top,
                   highlight_color,
                   modelView,
                   projection,
                   rect(),
                   zNear);

        // Bottom face
        drawLine3D(painter,
                   p1_bot,
                   p2_bot,
                   highlight_color,
                   modelView,
                   projection,
                   rect(),
                   zNear);
        drawLine3D(painter,
                   p2_bot,
                   p3_bot,
                   highlight_color,
                   modelView,
                   projection,
                   rect(),
                   zNear);
        drawLine3D(painter,
                   p3_bot,
                   p4_bot,
                   highlight_color,
                   modelView,
                   projection,
                   rect(),
                   zNear);
        drawLine3D(painter,
                   p4_bot,
                   p1_bot,
                   highlight_color,
                   modelView,
                   projection,
                   rect(),
                   zNear);

        // Vertical edges
        drawLine3D(painter,
                   p1_top,
                   p1_bot,
                   highlight_color,
                   modelView,
                   projection,
                   rect(),
                   zNear);
        drawLine3D(painter,
                   p2_top,
                   p2_bot,
                   highlight_color,
                   modelView,
                   projection,
                   rect(),
                   zNear);
        drawLine3D(painter,
                   p3_top,
                   p3_bot,
                   highlight_color,
                   modelView,
                   projection,
                   rect(),
                   zNear);
        drawLine3D(painter,
                   p4_top,
                   p4_bot,
                   highlight_color,
                   modelView,
                   projection,
                   rect(),
                   zNear);
      }
    }
  }
}

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
    return;
  }

  if (p1_visible != p2_visible) {
    // Prevent division by zero when z values are extremely close to kClipZ
    const float denominator = p2_view.z() - p1_view.z();
    float t = 0.5f;
    if (std::abs(denominator) > 1e-6f) {
      t = (kClipZ - p1_view.z()) / denominator;
    }
    const QVector3D intersection = p1_view + (p2_view - p1_view) * t;

    if (!p1_visible) {
      p1_view = intersection;
    } else {
      p2_view = intersection;
    }
  }

  const QVector3D p1_ndc = projection.map(p1_view);
  const QVector3D p2_ndc = projection.map(p2_view);

  const float w = viewport.width();
  const float h = viewport.height();

  const QPointF s1((p1_ndc.x() + 1.0f) * 0.5f * w,
                   (1.0f - p1_ndc.y()) * 0.5f * h);
  const QPointF s2((p2_ndc.x() + 1.0f) * 0.5f * w,
                   (1.0f - p2_ndc.y()) * 0.5f * h);

  painter.setPen(QPen(color, kLine3DWidth));
  painter.drawLine(s1, s2);
}

std::optional<QPolygonF> Chiplet3DWidget::projectAndClipFace(
    const Face& face,
    const QMatrix4x4& modelView,
    const QMatrix4x4& projection,
    float width,
    float height,
    float kClipZ) const
{
  std::vector<QVector3D> face_view_points;
  face_view_points.reserve(4);
  for (int i = 0; i < 4; ++i) {
    face_view_points.push_back(modelView * vertices_[face.indices[i]].position);
  }

  std::vector<QVector3D> clipped_points;
  for (size_t i = 0; i < face_view_points.size(); ++i) {
    const QVector3D& p1 = face_view_points[i];
    const QVector3D& p2 = face_view_points[(i + 1) % face_view_points.size()];

    bool p1_visible = p1.z() < kClipZ;
    bool p2_visible = p2.z() < kClipZ;

    if (p1_visible) {
      clipped_points.push_back(p1);
    }

    if (p1_visible != p2_visible) {
      // Prevent division by zero when z values are extremely close to kClipZ
      const float denominator = p2.z() - p1.z();
      float t = 0.5f;
      if (std::abs(denominator) > 1e-6f) {
        t = (kClipZ - p1.z()) / denominator;
      }
      QVector3D intersection = p1 + (p2 - p1) * t;
      clipped_points.push_back(intersection);
    }
  }

  if (clipped_points.empty()) {
    return std::nullopt;
  }

  QPolygonF poly;
  for (const auto& p_view : clipped_points) {
    QVector3D p_ndc = projection.map(p_view);
    poly << QPointF((p_ndc.x() + 1.0f) * 0.5f * width,
                    (1.0f - p_ndc.y()) * 0.5f * height);
  }

  return poly;
}

void Chiplet3DWidget::drawFace3D(QPainter& painter,
                                 const Face& face,
                                 const QMatrix4x4& modelView,
                                 const QMatrix4x4& projection,
                                 const QRect& viewport,
                                 float zNear)
{
  const float kClipZ = -zNear;

  auto polygon_opt = projectAndClipFace(
      face, modelView, projection, viewport.width(), viewport.height(), kClipZ);
  if (!polygon_opt) {
    return;
  }
  const QPolygonF& polygon_buffer = *polygon_opt;

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
      color = QColor(hc.r, hc.g, hc.b, 255);
      pen_width = 3;
    } else if (selected_.contains(face.selection)) {
      color = QColor(Painter::kHighlight.r,
                     Painter::kHighlight.g,
                     Painter::kHighlight.b,
                     255);
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
                     255);
    }
  }

  painter.setBrush(QBrush(color));
  painter.setPen(QPen(color.darker(), pen_width));
  painter.drawPolygon(polygon_buffer);
}

void Chiplet3DWidget::mousePressEvent(QMouseEvent* e)
{
  mouse_press_position_ = QVector2D(e->localPos());
}

void Chiplet3DWidget::mouseMoveEvent(QMouseEvent* e)
{
  const QVector2D diff = QVector2D(e->localPos()) - mouse_press_position_;
  if (diff.lengthSquared() < kMouseClickTolerance) {
    return;
  }
  mouse_press_position_ = QVector2D(e->localPos());

  if (e->buttons() & Qt::LeftButton) {
    const QVector3D n = QVector3D(diff.y(), diff.x(), 0.0).normalized();
    const float angle = diff.length() / kRotationSensitivity;
    QQuaternion new_rotation
        = QQuaternion::fromAxisAndAngle(n, angle) * rotation_;

    // Constrain rotation to prevent viewing from below (z < 0)
    if ((new_rotation * QVector3D(0.0f, 0.0f, 1.0f)).z() >= 0.0f) {
      rotation_ = new_rotation;
    } else {
      // If combined rotation goes below, try applying only horizontal (yaw)
      // rotation
      QQuaternion yaw_rotation
          = QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f),
                                          diff.x() / kRotationSensitivity)
            * rotation_;
      if ((yaw_rotation * QVector3D(0.0f, 0.0f, 1.0f)).z() >= 0.0f) {
        rotation_ = yaw_rotation;
      }
    }
  } else if (e->buttons() & Qt::RightButton) {
    const float scale = distance_ * kPanSensitivity;
    pan_x_ += diff.x() * scale;
    pan_y_ -= diff.y() * scale;
  }
  update();
}

void Chiplet3DWidget::mouseReleaseEvent(QMouseEvent* e)
{
  const QVector2D diff = QVector2D(e->localPos()) - mouse_press_position_;
  if (std::abs(diff.x()) + std::abs(diff.y()) > 5.0f) {
    return;
  }

  if (e->button() != Qt::LeftButton) {
    return;
  }

  // Hit-test
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

  const qreal aspect = qreal(width()) / qreal(height() ? height() : 1);
  QMatrix4x4 projection;
  projection.perspective(45.0f, aspect, zNear, zFar);

  QMatrix4x4 modelView;
  modelView.translate(pan_x_, pan_y_, -distance_);
  modelView.rotate(rotation_);

  const float w = width();
  const float h = height();
  const float kClipZ = -zNear;

  const QPointF click_pos = e->localPos();

  for (const auto& sorted_face : std::ranges::reverse_view(sorted_faces_)) {
    const Face* face = sorted_face.face;
    if (!face || !face->selection) {
      continue;
    }

    auto poly_opt
        = projectAndClipFace(*face, modelView, projection, w, h, kClipZ);
    if (!poly_opt) {
      continue;
    }

    if (poly_opt->containsPoint(click_pos, Qt::OddEvenFill)) {
      emit selected(face->selection);
      break;
    }
  }
}

void Chiplet3DWidget::wheelEvent(QWheelEvent* e)
{
  if (e->angleDelta().y() > 0) {
    distance_ *= kZoomInFactor;
  } else {
    distance_ *= kZoomOutFactor;
  }
  const float min_dist
      = std::max(bounding_radius_ * kMinDistanceFraction, 1.0f);
  const float max_dist
      = std::max(bounding_radius_ * kMaxDistanceFraction, min_dist);
  distance_ = std::clamp(distance_, min_dist, max_dist);
  update();
}

}  // namespace gui
