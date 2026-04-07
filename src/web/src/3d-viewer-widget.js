// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import * as THREE from 'https://esm.sh/three@0.160.0';

// Navigation constants matching the Qt GUI (chiplet3DWidget.cpp)
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

export class ThreeDViewerWidget {
    constructor(container, app) {
        this._app = app;
        this._container = container;
        this._element = document.createElement('div');
        this._element.className = '3d-viewer-widget';
        this._element.style.width = '100%';
        this._element.style.height = '100%';
        this._element.style.position = 'relative';
        this._element.style.backgroundColor = 'var(--bg-panel)';

        container.element.appendChild(this._element);

        this._scene = new THREE.Scene();
        this._scene.background = new THREE.Color(0x000000);

        this._camera = new THREE.PerspectiveCamera(45, 1, 10, 100000);
        this._camera.position.set(0, 0, 1000);

        this._renderer = new THREE.WebGLRenderer({ antialias: true });
        this._renderer.setPixelRatio(window.devicePixelRatio);
        this._renderer.setSize(container.width, container.height);
        this._element.appendChild(this._renderer.domElement);

        // Camera navigation state — matches Qt chiplet3DWidget exactly:
        // quaternion-based rotation, distance-based zoom, screen-space pan.
        this._rotation = new THREE.Quaternion();
        this._distance = 1000;
        this._panX = 0;
        this._panY = 0;
        this._boundingRadius = 1;
        this._sceneCenter = new THREE.Vector3();

        // Mouse tracking
        this._mouseDown = false;
        this._mouseButton = -1;
        this._lastMousePos = { x: 0, y: 0 };

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

        this._animationId = null;
        this.animate = this.animate.bind(this);

        container.on('resize', this.onResize.bind(this));
        container.on('open', this.onOpen.bind(this));
        container.on('destroy', this.onDestroy.bind(this));

        // Group for design objects
        this._designGroup = new THREE.Group();
        this._scene.add(this._designGroup);

        this._chipletMeshes = [];  // tracks chiplet mesh → data for scene rebuild checks

        this.loadData();
    }

    // ─── Mouse Handlers (matching Qt chiplet3DWidget) ──────────────────

    _setupMouseHandlers() {
        const canvas = this._renderer.domElement;

        canvas.addEventListener('mousedown', (e) => {
            this._mouseDown = true;
            this._mouseButton = e.button;
            this._lastMousePos.x = e.clientX;
            this._lastMousePos.y = e.clientY;
            e.preventDefault();
        });

        canvas.addEventListener('contextmenu', (e) => e.preventDefault());

        this._onMouseMove = (e) => {
            if (!this._mouseDown) return;

            const dx = e.clientX - this._lastMousePos.x;
            const dy = e.clientY - this._lastMousePos.y;
            this._lastMousePos.x = e.clientX;
            this._lastMousePos.y = e.clientY;

            if (dx === 0 && dy === 0) return;

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
            const minDist = Math.max(this._boundingRadius * kMinDistanceFraction, 1.0);
            const maxDist = Math.max(this._boundingRadius * kMaxDistanceFraction, minDist);
            this._distance = Math.max(minDist, Math.min(maxDist, this._distance));
            this._updateCamera();
        }, { passive: false });
    }

    // Quaternion-based rotation matching Qt: axis perpendicular to mouse
    // movement in screen space, angle proportional to drag distance.
    _handleRotate(dx, dy) {
        const length = Math.sqrt(dx * dx + dy * dy);
        if (length < 0.001) return;

        // Qt: axis = normalize(diff.y, diff.x, 0)
        const axis = new THREE.Vector3(dy / length, dx / length, 0);
        const angleDeg = length / kRotationSensitivity;
        const deltaQuat = new THREE.Quaternion().setFromAxisAngle(axis, angleDeg * DEG2RAD);

        const newRotation = deltaQuat.clone().multiply(this._rotation);

        // Constraint: prevent viewing from below (Z must stay up)
        const zUp = new THREE.Vector3(0, 0, 1).applyQuaternion(newRotation);
        if (zUp.z >= 0) {
            this._rotation.copy(newRotation);
        } else {
            // Try yaw-only rotation (horizontal component)
            const yawAxis = new THREE.Vector3(0, 1, 0);
            const yawAngle = (dx / kRotationSensitivity) * DEG2RAD;
            const yawQuat = new THREE.Quaternion().setFromAxisAngle(yawAxis, yawAngle);
            const yawRotation = yawQuat.clone().multiply(this._rotation);
            const zUpYaw = new THREE.Vector3(0, 0, 1).applyQuaternion(yawRotation);
            if (zUpYaw.z >= 0) {
                this._rotation.copy(yawRotation);
            }
        }
    }

    // Pan matching Qt: screen-space offset scaled by distance.
    _handlePan(dx, dy) {
        const scale = this._distance * kPanSensitivity;
        this._panX += dx * scale;
        this._panY -= dy * scale;  // Y inverted, matching Qt
    }

    // Reconstruct camera transform from state, matching Qt modelView:
    //   modelView = translate(panX, panY, -distance) * rotate(rotation)
    _updateCamera() {
        // Dynamic near/far planes matching Qt (depends on distance and bounds)
        const safeSize = Math.max(this._boundingRadius, 1000);
        const zNear = Math.max(this._distance - safeSize * kNearFarFactor, kMinZNear);
        let zFar = this._distance + safeSize * kNearFarFactor;
        if (zFar < zNear + 100) {
            zFar = zNear + 10000;
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
    }

    // ─── Lifecycle ─────────────────────────────────────────────────────

    onOpen() {
        this.onResize();
        if (!this._animationId) {
            this.animate();
        }
        if (this._chipletMeshes.length === 0) {
            this.loadData();
        }
    }

    onDestroy() {
        if (this._animationId) {
            cancelAnimationFrame(this._animationId);
            this._animationId = null;
        }
        this.clearHighlightDRC();
        if (this._onMouseMove) {
            window.removeEventListener('mousemove', this._onMouseMove);
        }
        if (this._onMouseUp) {
            window.removeEventListener('mouseup', this._onMouseUp);
        }
        if (this._renderer) {
            this._renderer.dispose();
        }
    }

    onResize() {
        const width = this._container.width;
        const height = this._container.height;
        if (width === 0 || height === 0) return;

        this._camera.aspect = width / height;
        this._camera.updateProjectionMatrix();
        this._renderer.setSize(width, height);
        this.render();
    }

    animate() {
        this._animationId = requestAnimationFrame(this.animate);
        this._renderer.render(this._scene, this._camera);
    }

    render() {
        if (this._renderer && this._scene && this._camera) {
            this._renderer.render(this._scene, this._camera);
        }
    }

    // ─── Data Loading ──────────────────────────────────────────────────

    async loadData() {
        for (const sel of ['.three-d-error', '.three-d-info']) {
            const prev = this._element.querySelector(sel);
            if (prev) prev.remove();
        }

        try {
            await this._app.websocketManager.readyPromise;
            const data = await this._app.websocketManager.request({ type: 'get_3d_data' });
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
            console.error('Failed to load 3D data:', err);
            this._showError(err.message || String(err));
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
        this._showOverlay('three-d-error',
            'Failed to load 3D data: ' + message, 'var(--error)');
    }

    _showInfo(message) {
        this._showOverlay('three-d-info', message, 'var(--fg-muted, #888)');
    }

    // ─── Scene Construction ────────────────────────────────────────────

    buildScene(data) {
        // Clear existing objects
        this._chipletMeshes = [];
        while (this._designGroup.children.length > 0) {
            const child = this._designGroup.children[0];
            this._designGroup.remove(child);
            if (child.geometry) child.geometry.dispose();
            if (child.material && !child.material._shared) child.material.dispose();
        }

        if (!data || !data.chiplets || data.chiplets.length === 0) return;

        let bounds = {
            minX: Infinity, minY: Infinity, minZ: Infinity,
            maxX: -Infinity, maxY: -Infinity, maxZ: -Infinity
        };

        const edgeMaterial = new THREE.LineBasicMaterial({ color: 0x000000, linewidth: 1 });
        edgeMaterial._shared = true;

        const dbu = this._app.techData?.dbu_per_micron || 1000;

        // Pre-compute Z offsets to separate chiplets that share the same Z
        // AND overlap in XY.  Chiplets at the same z that don't overlap
        // (e.g. side-by-side) keep their original position.
        //
        // Algorithm: for each z-group, greedily assign chiplets to the
        // lowest slot where they don't overlap with any chiplet already
        // in that slot.
        const zGroups = new Map(); // z -> [{idx, chiplet}]
        data.chiplets.forEach((chiplet, idx) => {
            const zKey = chiplet.z;
            if (!zGroups.has(zKey)) zGroups.set(zKey, []);
            zGroups.get(zKey).push({ idx, chiplet });
        });

        const zOffsets = new Array(data.chiplets.length).fill(0);

        for (const group of zGroups.values()) {
            if (group.length <= 1) continue;
            // slots[i] = list of chiplet rects placed in slot i
            const slots = [];
            for (const { idx, chiplet } of group) {
                const ax1 = chiplet.x;
                const ay1 = chiplet.y;
                const ax2 = chiplet.x + chiplet.width;
                const ay2 = chiplet.y + chiplet.height;

                let placed = false;
                for (let s = 0; s < slots.length; s++) {
                    const overlaps = slots[s].some(r =>
                        ax1 < r.x2 && ax2 > r.x1 &&
                        ay1 < r.y2 && ay2 > r.y1
                    );
                    if (!overlaps) {
                        slots[s].push({ x1: ax1, y1: ay1, x2: ax2, y2: ay2 });
                        zOffsets[idx] = s;
                        placed = true;
                        break;
                    }
                }
                if (!placed) {
                    slots.push([{ x1: ax1, y1: ay1, x2: ax2, y2: ay2 }]);
                    zOffsets[idx] = slots.length - 1;
                }
            }
        }

        // Build chiplets — Z coordinates scaled by kZScale to exaggerate
        // vertical stacking (matching Qt kLayerGapFactor = 2.0)
        data.chiplets.forEach((chiplet, idx) => {
            const width = chiplet.width / dbu;
            const height = chiplet.height / dbu;
            const depth = ((chiplet.thickness || 10000) / dbu) * kZScale;

            const colorHex = CHIPLET_PALETTE[idx % CHIPLET_PALETTE.length];
            const material = new THREE.MeshPhongMaterial({
                color: colorHex,
                transparent: false,
                opacity: 1.0,
                side: THREE.DoubleSide,
                shininess: 40,
            });

            const geometry = new THREE.BoxGeometry(width, height, depth);
            const mesh = new THREE.Mesh(geometry, material);

            // Position at chiplet center (Z scaled).
            // Apply stacking offset when multiple chiplets share the same z.
            const cx = (chiplet.x / dbu) + width / 2;
            const cy = (chiplet.y / dbu) + height / 2;
            const stackGap = depth * 0.15;
            const cz = ((chiplet.z / dbu) * kZScale)
                      + depth / 2
                      + zOffsets[idx] * (depth + stackGap);

            mesh.position.set(cx, cy, cz);
            this._designGroup.add(mesh);

            this._chipletMeshes.push({ mesh, data: chiplet });

            // Edges for depth clarity
            const edges = new THREE.EdgesGeometry(geometry);
            const line = new THREE.LineSegments(edges, edgeMaterial);
            line.position.set(cx, cy, cz);
            this._designGroup.add(line);

            // Update bounds (in scaled coordinates)
            bounds.minX = Math.min(bounds.minX, chiplet.x / dbu);
            bounds.minY = Math.min(bounds.minY, chiplet.y / dbu);
            bounds.minZ = Math.min(bounds.minZ, cz - depth / 2);
            bounds.maxX = Math.max(bounds.maxX, (chiplet.x / dbu) + width);
            bounds.maxY = Math.max(bounds.maxY, (chiplet.y / dbu) + height);
            bounds.maxZ = Math.max(bounds.maxZ, cz + depth / 2);
        });

        if (bounds.minX === Infinity) return;

        this._sceneBounds = bounds;

        // Grid on the XY plane at the base
        const sizeX = bounds.maxX - bounds.minX;
        const sizeY = bounds.maxY - bounds.minY;
        const gridSize = Math.max(sizeX, sizeY) * 1.4;
        const gridDivisions = 10;
        const gridHelper = new THREE.GridHelper(gridSize, gridDivisions, 0x888888, 0x444444);
        gridHelper.rotation.x = Math.PI / 2;
        gridHelper.position.set(
            (bounds.minX + bounds.maxX) / 2,
            (bounds.minY + bounds.maxY) / 2,
            bounds.minZ - 1
        );
        this._designGroup.add(gridHelper);

        // Compute bounding sphere radius (matching Qt)
        const dx = bounds.maxX - bounds.minX;
        const dy = bounds.maxY - bounds.minY;
        const dz = bounds.maxZ - bounds.minZ;
        this._boundingRadius = Math.sqrt(dx * dx + dy * dy + dz * dz) / 2;

        // Scene center (orbit pivot)
        this._sceneCenter.set(
            (bounds.minX + bounds.maxX) / 2,
            (bounds.minY + bounds.maxY) / 2,
            (bounds.minZ + bounds.maxZ) / 2
        );

        // Initial distance (matching Qt kInitialDistanceFactor = 3.0)
        this._distance = this._boundingRadius * kInitialDistanceFactor;

        // Initial rotation matching Qt: pitch -45° around X, then yaw +45° around Z
        const pitch = new THREE.Quaternion().setFromAxisAngle(
            new THREE.Vector3(1, 0, 0), -45 * DEG2RAD);
        const yaw = new THREE.Quaternion().setFromAxisAngle(
            new THREE.Vector3(0, 0, 1), 45 * DEG2RAD);
        this._rotation = pitch.multiply(yaw);

        // Reset pan
        this._panX = 0;
        this._panY = 0;

        this._updateCamera();

        // Start animation if not already running
        if (!this._animationId) {
            this.animate();
        }
    }

    // ─── DRC Highlighting ──────────────────────────────────────────────

    clearHighlightDRC() {
        if (this._drcBlinkInterval) {
            clearInterval(this._drcBlinkInterval);
            this._drcBlinkInterval = null;
        }
        if (this._drcMeshGroup) {
            this._scene.remove(this._drcMeshGroup);
            this._drcMeshGroup.children.forEach(child => {
                if (child.geometry) child.geometry.dispose();
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
        this.clearHighlightDRC();
        if (!violations || violations.length === 0) return;

        const dbu = this._app.techData?.dbu_per_micron || 1000;

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
            color: 0xffff00,
            transparent: true,
            opacity: 1.0,
            depthTest: false,
            depthWrite: false
        });

        for (const violation of violations) {
            const rects = (violation.rects && violation.rects.length > 0)
                ? violation.rects : [violation.bbox];

            for (const rect of rects) {
                const [x1, y1, x2, y2, z1, z2] = rect;
                const width = Math.max((x2 - x1) / dbu, 0.001);
                const height = Math.max((y2 - y1) / dbu, 0.001);

                let currentZMin = zMin;
                let currentDepth = depth;
                if (rect.length >= 6 && z1 !== undefined && z2 !== undefined) {
                    currentZMin = (z1 / dbu) * kZScale;
                    const z2Scaled = (z2 / dbu) * kZScale;
                    currentDepth = Math.max(z2Scaled - currentZMin, 0.001);
                }

                const cx = (x1 / dbu) + width / 2;
                const cy = (y1 / dbu) + height / 2;
                const cz = currentZMin + currentDepth / 2;

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

        let visible = true;
        this._drcBlinkInterval = setInterval(() => {
            if (this._drcMeshGroup && this._drcMaterial) {
                visible = !visible;
                this._drcMaterial.opacity = visible ? 1.0 : 0.2;
                this.render();
            }
        }, 300);
    }

}
