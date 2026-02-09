// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include "chiplet3DWidget.h"

#include <GL/gl.h>

#include <QMouseEvent>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace {
constexpr float SCENE_RADIUS_FACTOR = 0.75f;
constexpr float INITIAL_DISTANCE_FACTOR = 3.0f;
constexpr float THICKNESS_FACTOR = 0.02f;
constexpr float LAYER_GAP_FACTOR = 2.0f;
constexpr float Z_FIT_FACTOR = 3.0f;
constexpr float MIN_DISTANCE_CHECK = 100.0f;
constexpr float DEFAULT_DISTANCE = 1000.0f;
constexpr float SAFE_SIZE_FACTOR = 1.0f;
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

  vertices_.clear();
  indices_lines_.clear();

  odb::Rect global_bbox;
  global_bbox.mergeInit();

  std::vector<std::tuple<odb::dbChip*, odb::dbTransform, int, int>> stack;
  stack.emplace_back(chip_, odb::dbTransform(), 0, 0);

  struct ChipData
  {
    odb::Rect die;               // Original die
    odb::Rect transformed_bbox;  // Transformed XY bounding box for collision
    odb::dbTransform xform;
    int depth;
    std::string name;
    uint64_t area;
    int real_z;
    int thickness;
    int assigned_z_level;
  };
  std::vector<ChipData> chips;

  while (!stack.empty()) {
    auto [curr_chip, curr_xform, depth, curr_z] = stack.back();
    stack.pop_back();

    odb::dbBlock* block = curr_chip->getBlock();
    if (block) {
      odb::Rect die = block->getDieArea();
      if (die.dx() > 0 && die.dy() > 0) {
        // Calculate transformed bbox for collision detection
        odb::Rect bbox;
        bbox.mergeInit();
        std::vector<odb::Point> pts = {{die.xMin(), die.yMin()},
                                       {die.xMax(), die.yMin()},
                                       {die.xMax(), die.yMax()},
                                       {die.xMin(), die.yMax()}};
        for (auto& p : pts) {
          curr_xform.apply(p);
          bbox.merge(p);
          global_bbox.merge(p);
        }

        int calculated_depth = curr_z / 100;

        chips.emplace_back(die,
                           bbox,
                           curr_xform,
                           calculated_depth,
                           block->getName(),
                           static_cast<uint64_t>(die.area()),
                           curr_z,
                           curr_chip->getThickness());
      }
    }

    for (auto inst : curr_chip->getChipInsts()) {
      odb::dbTransform next_xform = curr_xform;
      odb::dbTransform inst_xform = inst->getTransform();
      next_xform.concat(inst_xform);
      stack.emplace_back(inst->getMasterChip(),
                         next_xform,
                         depth + 1,
                         curr_z + inst->getLoc().z());
    }
  }

  if (chips.empty()) {
    if (logger_) {
      logger_->info(utl::GUI, 82, "[3D Viewer] No chips found.");
    }
    return;
  }

  // Auto-Stacking Algorithm
  // Sort by Z Ascending
  std::ranges::sort(chips, [](const ChipData& a, const ChipData& b) {
    return a.real_z < b.real_z;
  });

  std::vector<ChipData*> placed;
  int max_level = 0;

  for (size_t i = 0; i < chips.size(); ++i) {
    int level = 0;
    // If it's not the very first (largest) chip, start at level 1 to sit on top
    // of base
    if (i > 0) {
      level = 1;
    }

    bool collision = true;
    while (collision) {
      collision = false;
      for (auto* other : placed) {
        if (other->assigned_z_level == level) {
          // Check XY intersection
          if (chips[i].transformed_bbox.intersects(other->transformed_bbox)) {
            collision = true;
            break;
          }
        }
      }
      if (collision) {
        level++;
      }
    }
    chips[i].assigned_z_level = level;
    placed.push_back(&chips[i]);
    max_level = std::max(max_level, level);
  }

  // Center and Camera
  float cx = (global_bbox.xMin() + global_bbox.xMax()) / 2.0f;
  float cy = (global_bbox.yMin() + global_bbox.yMax()) / 2.0f;
  center_ = QVector3D(cx, cy, 0.0f);

  float max_dim = std::max(global_bbox.dx(), global_bbox.dy());
  scene_radius_ = max_dim * SCENE_RADIUS_FACTOR;
  distance_ = scene_radius_ * INITIAL_DISTANCE_FACTOR;
  float thickness = max_dim * THICKNESS_FACTOR;

  scene_height_ = (max_level + 1) * thickness * LAYER_GAP_FACTOR;  // Gap factor

  float z_fit_distance = scene_height_ * Z_FIT_FACTOR;
  distance_ = std::max(distance_, z_fit_distance);
  if (distance_ < MIN_DISTANCE_CHECK) {
    distance_ = DEFAULT_DISTANCE;
  }

  static const std::vector<QVector3D> palette = {
      {0.0f, 1.0f, 0.0f},  // Green
      {1.0f, 1.0f, 0.0f},  // Yellow
      {0.0f, 1.0f, 1.0f},  // Cyan
      {1.0f, 0.0f, 1.0f},  // Magenta
      {1.0f, 0.5f, 0.0f},  // Orange
      {0.5f, 0.5f, 1.0f},  // Light Blue
      {1.0f, 0.0f, 0.0f}   // Red
  };

  for (const auto& c : chips) {
    std::vector<odb::Point> pts = {{c.die.xMin(), c.die.yMin()},
                                   {c.die.xMax(), c.die.yMin()},
                                   {c.die.xMax(), c.die.yMax()},
                                   {c.die.xMin(), c.die.yMax()}};
    for (auto& p : pts) {
      c.xform.apply(p);
    }

    // Color by Depth (proportional to Z)
    QVector3D color = palette[c.assigned_z_level % palette.size()];

    // Z Calculation based on Real Z
    float z_level_base = static_cast<float>(c.real_z);
    float z_thickness = static_cast<float>(c.thickness);

    float z0 = z_level_base - center_.z();
    float z1 = z_level_base + z_thickness - center_.z();

    int base = vertices_.size();
    for (const auto& p : pts) {
      vertices_.push_back(
          {QVector3D(p.x() - center_.x(), p.y() - center_.y(), z0), color});
    }
    for (const auto& p : pts) {
      vertices_.push_back(
          {QVector3D(p.x() - center_.x(), p.y() - center_.y(), z1), color});
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

  float safe_size = std::max(scene_radius_, scene_height_ * SAFE_SIZE_FACTOR);
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
