// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import * as THREE from 'https://esm.sh/three@0.160.0';

import {getThemeColors} from './theme.js';

// Camera navigation tuning constants
const kRotationSensitivity = 2.0;
const kPanSensitivity = 0.002;
const kZoomInFactor = 0.9;
const kZoomOutFactor = 1.1;
const kMinDistanceFraction = 0.1;
const kMaxDistanceFraction = 20.0;
const kInitialDistanceFactor = 3.0;
const kZScale = 2.0;
const kNearFarFactor = 2.0;
const kMinZNear = 10.0;
const kZFarSafetyMargin = 10000;
const kStackGapFactor = 0.15;
const kDrcBlinkIntervalMs = 300;
const DEG2RAD = Math.PI / 180;

// Distinct color palette for chiplets (saturated, good contrast in 3D)
const CHIPLET_PALETTE = [
  0x4CAF50, // green
  0x2196F3, // blue
  0xFF9800, // orange
  0x9C27B0, // purple
  0x00BCD4, // cyan
  0xF44336, // red
  0x26A69A, // teal
  0x795548, // brown
  0xE91E63, // pink
  0x3F51B5, // indigo
];

// For each chiplet, compute an integer stacking slot so chiplets sharing the
// same z that overlap in XY are visually separated. Greedy lowest-slot fit:
// slot 0 is the natural z, slot 1 sits on top, etc.
function computeZOffsetSlots(chiplets) {
  const zOffsets = new Array(chiplets.length).fill(0);
  const zGroups = new Map();
  chiplets.forEach((chiplet, idx) => {
    const key = chiplet.z;
    if (!zGroups.has(key)) zGroups.set(key, []);
    zGroups.get(key).push({idx, chiplet});
  });

  for (const group of zGroups.values()) {
    if (group.length <= 1) continue;
    const slots = [];
    for (const {idx, chiplet} of group) {
      const ax1 = chiplet.x;
      const ay1 = chiplet.y;
      const ax2 = chiplet.x + chiplet.width;
      const ay2 = chiplet.y + chiplet.height;
      let placed = false;
      for (let s = 0; s < slots.length; s++) {
        const overlaps = slots[s].some(
            r => ax1 < r.x2 && ax2 > r.x1 && ay1 < r.y2 && ay2 > r.y1);
        if (!overlaps) {
          slots[s].push({x1: ax1, y1: ay1, x2: ax2, y2: ay2});
          zOffsets[idx] = s;
          placed = true;
          break;
        }
      }
      if (!placed) {
        slots.push([{x1: ax1, y1: ay1, x2: ax2, y2: ay2}]);
        zOffsets[idx] = slots.length - 1;
      }
    }
  }
  return zOffsets;
}

export class ThreeDViewerWidget {
  constructor(container, app) {
    this._app = app;
    this._container = container;
    this._element = document.createElement('div');
    this._element.className = 'three-d-viewer-widget';
    container.element.appendChild(this._element);

    // Canvas container fills the widget; gives the absolute-positioned
    // tooltip a relative origin to anchor against.
    this._canvasContainer = document.createElement('div');
    this._canvasContainer.className = 'three-d-canvas-container';
    this._element.appendChild(this._canvasContainer);

    // Tooltip shown on chiplet hover (raycasting).
    this._tooltip = document.createElement('div');
    this._tooltip.className = 'three-d-tooltip';
    this._canvasContainer.appendChild(this._tooltip);

    this._raycaster = new THREE.Raycaster();

    this._scene = new THREE.Scene();
    this._applyTheme();

    // Re-apply theme when document theme changes (light ↔ dark).
    this._themeObserver = new MutationObserver(() => {
      this._applyTheme();
      this.render();
    });
    this._themeObserver.observe(document.documentElement,
                                {attributes : true, attributeFilter : [ 'data-theme' ]});

    this._camera = new THREE.PerspectiveCamera(45, 1, 10, 100000);
    this._camera.position.set(0, 0, 1000);

    this._renderer = new THREE.WebGLRenderer({antialias : true});
    this._renderer.setPixelRatio(window.devicePixelRatio);
    this._renderer.setSize(container.width, container.height);
    this._canvasContainer.appendChild(this._renderer.domElement);

    // Camera navigation state: quaternion-based rotation, distance-based
    // zoom, and screen-space pan.
    this._rotation = new THREE.Quaternion();
    this._distance = 1000;
    this._panX = 0;
    this._panY = 0;
    this._boundingRadius = 1;
    this._sceneCenter = new THREE.Vector3();

    // Mouse tracking
    this._mouseDown = false;
    this._mouseButton = -1;
    this._lastMousePos = {x : 0, y : 0};

    this._setupMouseHandlers();

    // Lighting: ambient + two directional lights for better depth perception
    const ambientLight = new THREE.AmbientLight(0xffffff, 0.5);
    this._scene.add(ambientLight);

    const keyLight = new THREE.DirectionalLight(0xffffff, 0.8);
    keyLight.position.set(1, 1, 1).normalize();
    this._scene.add(keyLight);

    const fillLight = new THREE.DirectionalLight(0xffffff, 0.3);
    fillLight.position.set(-1, -0.5, 0.5).normalize();
    this._scene.add(fillLight);

    container.on('resize', this.onResize.bind(this));
    container.on('open', this.onOpen.bind(this));
    container.on('destroy', this.onDestroy.bind(this));

    // Group for design objects
    this._designGroup = new THREE.Group();
    this._scene.add(this._designGroup);

    this._chipletMeshes =
        []; // tracks chiplet mesh → data for scene rebuild checks
    this._loading = false;
    this._destroyed = false;

    // Shared across all chiplet edges; created lazily and disposed in
    // onDestroy(). Marked with _shared so _clearScene() does not dispose it
    // when meshes are removed during a rebuild.
    this._edgeMaterial = null;
  }

  // ─── Mouse Handlers ────────────────────────────────────────────────

  _setupMouseHandlers() {
    const canvas = this._renderer.domElement;

    canvas.addEventListener('mousedown', (e) => {
      this._mouseDown = true;
      this._mouseButton = e.button;
      this._lastMousePos.x = e.clientX;
      this._lastMousePos.y = e.clientY;
      canvas.focus();
      e.preventDefault();
    });

    canvas.addEventListener('contextmenu', (e) => e.preventDefault());

    this._onMouseMove = (e) => {
      if (!this._mouseDown)
        return;

      const dx = e.clientX - this._lastMousePos.x;
      const dy = e.clientY - this._lastMousePos.y;
      this._lastMousePos.x = e.clientX;
      this._lastMousePos.y = e.clientY;

      if (dx === 0 && dy === 0)
        return;

      if (this._mouseButton === 0) {
        this._handleRotate(dx, dy);
      } else if (this._mouseButton === 2) {
        this._handlePan(dx, dy);
      }

      this._updateCamera();
    };

    this._onMouseUp = (e) => {
      if (this._mouseDown && e.button === this._mouseButton) {
        this._mouseDown = false;
        this._mouseButton = -1;
      }
    };

    window.addEventListener('mousemove', this._onMouseMove);
    window.addEventListener('mouseup', this._onMouseUp);

    canvas.addEventListener('wheel', (e) => {
      e.preventDefault();
      if (e.deltaY < 0) {
        this._distance *= kZoomInFactor;
      } else {
        this._distance *= kZoomOutFactor;
      }
      const minDist =
          Math.max(this._boundingRadius * kMinDistanceFraction, 1.0);
      const maxDist =
          Math.max(this._boundingRadius * kMaxDistanceFraction, minDist);
      this._distance = Math.max(minDist, Math.min(maxDist, this._distance));
      this._updateCamera();
    }, {passive : false});

    canvas.tabIndex = 0;
    canvas.style.outline = 'none';

    canvas.addEventListener('keydown', (e) => {
      if (e.key.toLowerCase() === 'f') {
        this.resetCamera();
      }
    });

    canvas.addEventListener('dblclick', (e) => {
      e.preventDefault();
      this.resetCamera();
    });

    // Hover tooltip — only when not dragging.
    canvas.addEventListener('mousemove', (e) => {
      if (this._mouseDown) {
        this._hideTooltip();
        return;
      }
      this._updateTooltip(e);
    });

    canvas.addEventListener('mouseleave', () => { this._hideTooltip(); });
  }

  _updateTooltip(event) {
    if (!this._chipletMeshes.length) {
      this._hideTooltip();
      return;
    }

    const canvas = this._renderer.domElement;
    const rect = canvas.getBoundingClientRect();
    const ndc = new THREE.Vector2(
        ((event.clientX - rect.left) / rect.width) * 2 - 1,
        -(((event.clientY - rect.top) / rect.height) * 2 - 1));

    this._raycaster.setFromCamera(ndc, this._camera);
    const meshes = this._chipletMeshes.map((m) => m.mesh);
    const hits = this._raycaster.intersectObjects(meshes, false);
    if (hits.length === 0) {
      this._hideTooltip();
      return;
    }

    const hitMesh = hits[0].object;
    const entry = this._chipletMeshes.find((m) => m.mesh === hitMesh);
    if (!entry) {
      this._hideTooltip();
      return;
    }

    this._tooltip.textContent = entry.data.name;
    this._tooltip.style.display = 'block';
    // Position relative to canvas container (tooltip is its child).
    const containerRect = this._canvasContainer.getBoundingClientRect();
    this._tooltip.style.left = (event.clientX - containerRect.left + 12) + 'px';
    this._tooltip.style.top = (event.clientY - containerRect.top + 12) + 'px';
  }

  _hideTooltip() {
    if (this._tooltip) {
      this._tooltip.style.display = 'none';
    }
  }

  // Quaternion-based rotation: axis perpendicular to mouse movement in
  // screen space, angle proportional to drag distance.
  _handleRotate(dx, dy) {
    const length = Math.sqrt(dx * dx + dy * dy);
    if (length < 0.001)
      return;

    // Axis = normalize(diff.y, diff.x, 0)
    const axis = new THREE.Vector3(dy / length, dx / length, 0);
    const angleDeg = length / kRotationSensitivity;
    const deltaQuat =
        new THREE.Quaternion().setFromAxisAngle(axis, angleDeg * DEG2RAD);

    const newRotation = deltaQuat.clone().multiply(this._rotation);

    // Constraint: prevent viewing from below (Z must stay up)
    const zUp = new THREE.Vector3(0, 0, 1).applyQuaternion(newRotation);
    if (zUp.z >= 0) {
      this._rotation.copy(newRotation);
    } else {
      // Try yaw-only rotation (horizontal component)
      const yawAxis = new THREE.Vector3(0, 1, 0);
      const yawAngle = (dx / kRotationSensitivity) * DEG2RAD;
      const yawQuat =
          new THREE.Quaternion().setFromAxisAngle(yawAxis, yawAngle);
      const yawRotation = yawQuat.clone().multiply(this._rotation);
      const zUpYaw = new THREE.Vector3(0, 0, 1).applyQuaternion(yawRotation);
      if (zUpYaw.z >= 0) {
        this._rotation.copy(yawRotation);
      }
    }
  }

  // Pan: screen-space offset scaled by distance.
  _handlePan(dx, dy) {
    const scale = this._distance * kPanSensitivity;
    this._panX += dx * scale;
    this._panY -= dy * scale; // Y inverted (screen coords -> world)
  }

  // Reconstruct camera transform from state:
  //   modelView = translate(panX, panY, -distance) * rotate(rotation)
  _updateCamera() {
    // Dynamic near/far planes (depend on distance and scene bounds)
    const safeSize = Math.max(this._boundingRadius, 1000);
    const zNear =
        Math.max(this._distance - safeSize * kNearFarFactor, kMinZNear);
    let zFar = this._distance + safeSize * kNearFarFactor;
    if (zFar < zNear + 100) {
      zFar = zNear + kZFarSafetyMargin;
    }
    this._camera.near = zNear;
    this._camera.far = zFar;
    this._camera.updateProjectionMatrix();

    // Inverse of the rotation quaternion maps camera-local → world
    const qInv = this._rotation.clone().invert();

    // Camera position in world: qInv * (panX, panY, distance) + sceneCenter
    const pos = new THREE.Vector3(this._panX, this._panY, this._distance);
    pos.applyQuaternion(qInv);
    this._camera.position.copy(this._sceneCenter).add(pos);

    // Camera up: qInv * (0, 1, 0)
    this._camera.up.set(0, 1, 0).applyQuaternion(qInv);

    // Look-at target: sceneCenter + qInv * (panX, panY, 0)
    const target = new THREE.Vector3(this._panX, this._panY, 0);
    target.applyQuaternion(qInv);
    target.add(this._sceneCenter);
    this._camera.lookAt(target);

    this.render();
  }

  // ─── Lifecycle ─────────────────────────────────────────────────────

  onOpen() {
    if (this._destroyed)
      return;
    this.onResize();
    if (this._chipletMeshes.length === 0 && !this._loading) {
      this.loadData();
    }
  }

  onDestroy() {
    if (this._destroyed)
      return;
    this._destroyed = true;
    this._loading = false;

    this.clearHighlightDRC();

    if (this._themeObserver) {
      this._themeObserver.disconnect();
      this._themeObserver = null;
    }

    if (this._designGroup) {
      while (this._designGroup.children.length > 0) {
        const child = this._designGroup.children[0];
        this._designGroup.remove(child);
        if (child.geometry)
          child.geometry.dispose();
        if (child.material && !child.material._shared)
          child.material.dispose();
      }
    }

    if (this._edgeMaterial) {
      this._edgeMaterial.dispose();
      this._edgeMaterial = null;
    }

    if (this._onMouseMove) {
      window.removeEventListener('mousemove', this._onMouseMove);
      this._onMouseMove = null;
    }
    if (this._onMouseUp) {
      window.removeEventListener('mouseup', this._onMouseUp);
      this._onMouseUp = null;
    }
    if (this._renderer) {
      this._renderer.dispose();
      this._renderer = null;
    }

    if (this._app.threeDViewerWidget === this) {
      this._app.threeDViewerWidget = null;
    }
  }

  // Set distance, rotation (pitch -45°, yaw +45°) and pan to defaults.
  // Caller is responsible for calling _updateCamera() afterward.
  _setInitialCameraState() {
    this._distance = this._boundingRadius * kInitialDistanceFactor;

    const pitch = new THREE.Quaternion().setFromAxisAngle(
        new THREE.Vector3(1, 0, 0), -45 * DEG2RAD);
    const yaw = new THREE.Quaternion().setFromAxisAngle(
        new THREE.Vector3(0, 0, 1), 45 * DEG2RAD);
    this._rotation = pitch.multiply(yaw);

    this._panX = 0;
    this._panY = 0;
  }

  resetCamera() {
    if (!this._sceneBounds)
      return;
    this._setInitialCameraState();
    this._updateCamera();
  }

  onResize() {
    if (this._destroyed)
      return;
    // Read size from the canvas container — the Golden Layout container
    // includes the toolbar, which we must subtract.
    const rect = this._canvasContainer.getBoundingClientRect();
    const width = rect.width;
    const height = rect.height;
    if (width === 0 || height === 0)
      return;

    this._camera.aspect = width / height;
    this._camera.updateProjectionMatrix();
    this._renderer.setSize(width, height);
    this.render();
  }

  render() {
    if (this._destroyed)
      return;
    if (this._renderer && this._scene && this._camera) {
      this._renderer.render(this._scene, this._camera);
    }
  }

  // Read CSS custom properties and apply them to the THREE scene.
  _applyTheme() {
    const colors = getThemeColors();
    const bg = colors.bgMap || '#000000';
    this._scene.background = new THREE.Color(bg);
  }

  // ─── Data Loading ──────────────────────────────────────────────────

  async loadData() {
    if (this._loading || this._destroyed)
      return;
    this._loading = true;

    for (const sel of ['.three-d-error', '.three-d-info']) {
      const prev = this._element.querySelector(sel);
      if (prev)
        prev.remove();
    }

    try {
      await this._app.websocketManager.readyPromise;
      const data =
          await this._app.websocketManager.request({type : 'get_3d_data'});
      if (this._destroyed)
        return;
      if (data && data.error) {
        this._showError(data.error);
        return;
      }
      if (data && data.info) {
        this._showInfo(data.info);
        return;
      }
      this.buildScene(data);
    } catch (err) {
      if (this._destroyed)
        return;
      console.error('Failed to load 3D data:', err);
      this._showError(err.message || String(err));
    } finally {
      this._loading = false;
    }
  }

  _showOverlay(className, text, color) {
    const div = document.createElement('div');
    div.className = className;
    div.style.cssText =
        'position:absolute;top:50%;left:50%;transform:translate(-50%,-50%);' +
        'font-family:monospace;text-align:center;';
    div.style.color = color;
    div.textContent = text;
    this._element.appendChild(div);
  }

  _showError(message) {
    this._showOverlay('three-d-error', 'Failed to load 3D data: ' + message,
                      'var(--error)');
  }

  _showInfo(message) {
    this._showOverlay('three-d-info', message, 'var(--fg-muted, #888)');
  }

  // ─── Scene Construction ────────────────────────────────────────────

  buildScene(data) {
    if (this._destroyed)
      return;
    this._clearScene();

    if (!data || !data.chiplets || data.chiplets.length === 0)
      return;

    const dbu = this._app.getDbuPerMicron();
    const bounds = this._buildChiplets(data.chiplets, dbu);
    if (bounds.minX === Infinity)
      return;

    this._buildGrid(bounds);
    this._setSceneBoundsAndCenter(bounds);
    this._setInitialCameraState();
    this._updateCamera();
  }

  _clearScene() {
    this._chipletMeshes = [];
    while (this._designGroup.children.length > 0) {
      const child = this._designGroup.children[0];
      this._designGroup.remove(child);
      if (child.geometry)
        child.geometry.dispose();
      if (child.material && !child.material._shared)
        child.material.dispose();
    }
    this._hideTooltip();
  }

  _buildChiplets(chiplets, dbu) {
    const bounds = {
      minX : Infinity,
      minY : Infinity,
      minZ : Infinity,
      maxX : -Infinity,
      maxY : -Infinity,
      maxZ : -Infinity
    };

    if (!this._edgeMaterial) {
      this._edgeMaterial =
          new THREE.LineBasicMaterial({color : 0x000000, linewidth : 1});
      this._edgeMaterial._shared = true;
    }

    // Separate chiplets that share the same Z and overlap in XY by stacking
    // them in slots. See computeZOffsetSlots().
    const zOffsets = computeZOffsetSlots(chiplets);

    chiplets.forEach((chiplet, idx) => {
      const width = chiplet.width / dbu;
      const height = chiplet.height / dbu;
      const depth = (chiplet.thickness / dbu) * kZScale;

      const colorHex = CHIPLET_PALETTE[idx % CHIPLET_PALETTE.length];
      const material = new THREE.MeshPhongMaterial({
        color : colorHex,
        transparent : false,
        opacity : 1.0,
        side : THREE.DoubleSide,
        shininess : 40,
      });

      const geometry = new THREE.BoxGeometry(width, height, depth);
      const mesh = new THREE.Mesh(geometry, material);

      const cx = (chiplet.x / dbu) + width / 2;
      const cy = (chiplet.y / dbu) + height / 2;
      const stackGap = depth * kStackGapFactor;
      const cz = ((chiplet.z / dbu) * kZScale) + depth / 2 +
                 zOffsets[idx] * (depth + stackGap);

      mesh.position.set(cx, cy, cz);
      this._designGroup.add(mesh);
      this._chipletMeshes.push({mesh, data : chiplet, color : colorHex});

      const edges = new THREE.EdgesGeometry(geometry);
      const line = new THREE.LineSegments(edges, this._edgeMaterial);
      line.position.set(cx, cy, cz);
      this._designGroup.add(line);

      bounds.minX = Math.min(bounds.minX, chiplet.x / dbu);
      bounds.minY = Math.min(bounds.minY, chiplet.y / dbu);
      bounds.minZ = Math.min(bounds.minZ, cz - depth / 2);
      bounds.maxX = Math.max(bounds.maxX, (chiplet.x / dbu) + width);
      bounds.maxY = Math.max(bounds.maxY, (chiplet.y / dbu) + height);
      bounds.maxZ = Math.max(bounds.maxZ, cz + depth / 2);
    });

    return bounds;
  }

  _buildGrid(bounds) {
    const sizeX = bounds.maxX - bounds.minX;
    const sizeY = bounds.maxY - bounds.minY;
    const gridSize = Math.max(sizeX, sizeY) * 1.4;
    const gridDivisions = 10;
    const gridHelper =
        new THREE.GridHelper(gridSize, gridDivisions, 0x888888, 0x444444);
    gridHelper.rotation.x = Math.PI / 2;
    gridHelper.position.set((bounds.minX + bounds.maxX) / 2,
                            (bounds.minY + bounds.maxY) / 2, bounds.minZ - 1);
    this._designGroup.add(gridHelper);
  }

  _setSceneBoundsAndCenter(bounds) {
    this._sceneBounds = bounds;

    const dx = bounds.maxX - bounds.minX;
    const dy = bounds.maxY - bounds.minY;
    const dz = bounds.maxZ - bounds.minZ;
    this._boundingRadius = Math.sqrt(dx * dx + dy * dy + dz * dz) / 2;

    this._sceneCenter.set((bounds.minX + bounds.maxX) / 2,
                          (bounds.minY + bounds.maxY) / 2,
                          (bounds.minZ + bounds.maxZ) / 2);
  }

  // ─── DRC Highlighting ──────────────────────────────────────────────

  // Given a DRC point (z, cx, cy) in DBU, return the extra Z displacement
  // applied to the chiplet it belongs to (due to stacking in buildScene), so
  // that highlights land on the rendered chiplet, not its nominal z.
  _stackOffsetForPoint(z, cx, cy, dbu) {
    for (const {mesh, data: chiplet} of this._chipletMeshes) {
      if (z !== chiplet.z) continue;
      if (cx < chiplet.x || cx > chiplet.x + chiplet.width) continue;
      if (cy < chiplet.y || cy > chiplet.y + chiplet.height) continue;
      const chipDepth = (chiplet.thickness / dbu) * kZScale;
      const originalCZ = ((chiplet.z / dbu) * kZScale) + chipDepth / 2;
      return mesh.position.z - originalCZ;
    }
    return 0;
  }

  clearHighlightDRC() {
    if (this._drcBlinkInterval) {
      clearInterval(this._drcBlinkInterval);
      this._drcBlinkInterval = null;
    }
    if (this._drcMeshGroup) {
      this._scene.remove(this._drcMeshGroup);
      this._drcMeshGroup.children.forEach(child => {
        if (child.geometry)
          child.geometry.dispose();
      });
      if (this._drcMaterial) {
        this._drcMaterial.dispose();
        this._drcMaterial = null;
      }
      this._drcMeshGroup = null;
      this.render();
    }
  }

  highlightDRC(violations) {
    if (this._destroyed)
      return;
    this.clearHighlightDRC();
    if (!violations || violations.length === 0)
      return;

    const dbu = this._app.getDbuPerMicron();

    let zMin = 0;
    let zMax = (10000 / dbu) * kZScale;
    if (this._sceneBounds && this._sceneBounds.minZ !== Infinity) {
      zMin = this._sceneBounds.minZ;
      zMax = this._sceneBounds.maxZ;
    }
    zMin -= 1;
    zMax += 1;
    const depth = Math.max(zMax - zMin, 1);

    this._drcMeshGroup = new THREE.Group();

    this._drcMaterial = new THREE.LineBasicMaterial({
      color : 0xffff00,
      transparent : true,
      opacity : 1.0,
      depthTest : false,
      depthWrite : false
    });

    for (const violation of violations) {
      const rects = (violation.rects && violation.rects.length > 0)
                        ? violation.rects
                        : [ violation.bbox ];

      for (const rect of rects) {
        const [x1, y1, x2, y2, z1, z2] = rect;
        const width = Math.max((x2 - x1) / dbu, 0.001);
        const height = Math.max((y2 - y1) / dbu, 0.001);

        let currentZMin = zMin;
        let currentDepth = depth;
        let czOffset = 0;

        if (rect.length >= 6 && z1 !== undefined && z2 !== undefined) {
          currentZMin = (z1 / dbu) * kZScale;
          const z2Scaled = (z2 / dbu) * kZScale;
          currentDepth = Math.max(z2Scaled - currentZMin, 0.001);
          czOffset = this._stackOffsetForPoint(
              z1, (x1 + x2) / 2, (y1 + y2) / 2, dbu);
        }

        const cx = (x1 / dbu) + width / 2;
        const cy = (y1 / dbu) + height / 2;
        const cz = currentZMin + currentDepth / 2 + czOffset;

        const boxGeometry = new THREE.BoxGeometry(width, height, currentDepth);
        const edgesGeometry = new THREE.EdgesGeometry(boxGeometry);
        boxGeometry.dispose();
        const line = new THREE.LineSegments(edgesGeometry, this._drcMaterial);
        line.position.set(cx, cy, cz);
        line.renderOrder = 999;
        this._drcMeshGroup.add(line);
      }
    }

    this._drcMeshGroup.renderOrder = 999;
    this._scene.add(this._drcMeshGroup);
    this.render();

    let visible = true;
    this._drcBlinkInterval = setInterval(() => {
      if (this._drcMeshGroup && this._drcMaterial) {
        visible = !visible;
        this._drcMaterial.opacity = visible ? 1.0 : 0.2;
        this.render();
      }
    }, kDrcBlinkIntervalMs);
  }
}
