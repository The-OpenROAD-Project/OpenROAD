// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import {it} from 'node:test';
import assert from 'node:assert/strict';
import * as THREE from '../../src/three-subset.js';
import {createInterconnectMeshes} from '../../src/interconnect3d.js';

globalThis.THREE = THREE;
const {ThreeDViewerWidget} = await import('../../src/3d-viewer-widget.js');

const data = {
  bumps: [[0, 10, 20, 30, 60, 100],
          [1, 100, 200, 120, 260, 400],
          [2, -100, -200, -80, -140, -500]],
  nets: [{name: 'stack/bus', bumps: [0, 1, 2]}],
};

function dispose(meshes) {
  for (const mesh of Object.values(meshes)) {
    if (!mesh) continue;
    mesh.dispose?.();
    mesh.geometry.dispose();
    mesh.material.dispose();
  }
}

it('uses real footprint dimensions, Z scale, and each endpoint stacking offset', () => {
  const meshes = createInterconnectMeshes(data, [0, 35, -20], 10, 2, THREE);
  assert.equal(meshes.bumps.count, 3);
  const matrix = new THREE.Matrix4();
  meshes.bumps.getMatrixAt(1, matrix);
  assert.deepEqual(new THREE.Vector3().setFromMatrixPosition(matrix).toArray(),
                   [11, 23, 115]);
  assert.deepEqual(new THREE.Vector3().setFromMatrixScale(matrix).toArray(),
                   [2, 6, 1]);
  assert.deepEqual(Array.from(meshes.links.geometry.attributes.position.array),
                   [2, 4, 20, 11, 23, 115, 2, 4, 20, -9, -17, -120]);
  assert.equal(meshes.bumps.geometry.type, 'PlaneGeometry');
  dispose(meshes);
});

it('supports an older server or a design without bumps or nets', () => {
  assert.deepEqual(createInterconnectMeshes({}, [], 1000, 2, THREE),
                   {bumps: null, links: null});
  const meshes = createInterconnectMeshes({bumps: data.bumps}, [], 10, 2, THREE);
  assert.equal(meshes.bumps.count, 3);
  assert.equal(meshes.links, null);
  dispose(meshes);
});

it('batches 20000 physical bumps and fanout connections into two objects', () => {
  const count = 20000;
  const stress = {
    bumps: Array.from({length: count}, (_, i) =>
        [i % 4, i * 10, i * 20, i * 10 + 4, i * 20 + 6, i * 30]),
    nets: [{name: 'fanout', bumps: Array.from({length: count}, (_, i) => i)}],
  };
  const meshes = createInterconnectMeshes(stress, [0, 10, 20, 30], 10, 2, THREE);
  assert.equal(meshes.bumps.count, count);
  assert.equal(meshes.links.geometry.attributes.position.count, (count - 1) * 2);
  const matrix = new THREE.Matrix4();
  meshes.bumps.getMatrixAt(count - 1, matrix);
  const actual = new THREE.Vector3().setFromMatrixPosition(matrix);
  assert.ok(Math.abs(actual.x - ((count - 1) + 0.2)) < 0.01);
  assert.equal(actual.z, (count - 1) * 6 + 30);
  dispose(meshes);
});

function widget() {
  const value = Object.create(ThreeDViewerWidget.prototype);
  Object.assign(value, {
    _app: {useTrueZ: false},
    _showBumps: true,
    _showLinks: true,
    _showChipBodies: true,
    _chipletMeshes: [],
    _chipletStackOffsets: [],
    _designGroup: new THREE.Group(),
    _hideTooltip() {},
    render() {},
  });
  return value;
}

it('visibility controls independently toggle bumps, links, and die bodies', () => {
  const value = widget();
  value._interconnectMeshes = createInterconnectMeshes(data, [], 10, 2, THREE);
  value._chipletMeshes = [{mesh: new THREE.Mesh()}];
  const overlay = value._createOptionsOverlay();
  const controls = [...overlay.querySelectorAll('label')];
  for (const [text, object] of [
    ['Bumps', value._interconnectMeshes.bumps],
    ['Net links', value._interconnectMeshes.links],
    ['Chip bodies', value._chipletMeshes[0].mesh],
  ]) {
    const checkbox = controls.find(label => label.textContent === text)
                        .querySelector('input');
    assert.equal(object.visible, true);
    checkbox.checked = false;
    checkbox.dispatchEvent(new Event('change'));
    assert.equal(object.visible, false);
    checkbox.checked = true;
    checkbox.dispatchEvent(new Event('change'));
    assert.equal(object.visible, true);
  }
  dispose(value._interconnectMeshes);
});

it('scene rebuild disposes instancing buffers and geometry', () => {
  const value = widget();
  value._buildInterconnects(data, 10);
  let disposed = 0;
  for (const mesh of value._designGroup.children) {
    mesh.geometry.addEventListener('dispose', () => ++disposed);
    mesh.material.addEventListener('dispose', () => ++disposed);
    if (mesh.isInstancedMesh) mesh.addEventListener('dispose', () => ++disposed);
  }
  value._clearScene();
  assert.equal(disposed, 5);
  assert.equal(value._designGroup.children.length, 0);
  assert.equal(value._interconnectMeshes, null);
});

function cameraWidget(bounds, aspect) {
  const value = widget();
  value._camera = new THREE.PerspectiveCamera(45, aspect, 10, 100000);
  value._rotation = new THREE.Quaternion();
  value._sceneCenter = new THREE.Vector3();
  value._setSceneBoundsAndCenter(bounds);
  return value;
}

function assertFits(value) {
  value._camera.updateMatrixWorld(true);
  const b = value._sceneBounds;
  for (const x of [b.minX, b.maxX]) {
    for (const y of [b.minY, b.maxY]) {
      for (const z of [b.minZ, b.maxZ]) {
        const corner = new THREE.Vector3(x, y, z).project(value._camera);
        assert.ok(Number.isFinite(corner.x + corner.y + corner.z));
        assert.ok(Math.abs(corner.x) <= 1, 'Horizontal clip: ' + corner.x);
        assert.ok(Math.abs(corner.y) <= 1, 'Vertical clip: ' + corner.y);
        assert.ok(Math.abs(corner.z) <= 1, 'Near/far clip: ' + corner.z);
      }
    }
  }
  assert.ok(Number.isFinite(value._distance) && value._distance > 0);
}

const fitBounds = [
  {minX: -4000, maxX: 6000, minY: -20, maxY: 20, minZ: -10, maxZ: 10},
  {minX: -25, maxX: 25, minY: -50, maxY: 50, minZ: -3000, maxZ: 7000},
  {minX: -20, maxX: 20, minY: -7000, maxY: 3000, minZ: 5, maxZ: 25},
  {minX: 1000000, maxX: 1000020, minY: -100, maxY: 20, minZ: 30, maxZ: 80},
  {minX: -100, maxX: 100, minY: -50, maxY: 50, minZ: 0, maxZ: 0},
  {minX: 7, maxX: 7, minY: -2, maxY: -2, minZ: 30, maxZ: 30},
];

it('reset fits wide, tall, flat, and degenerate bounds at desktop/mobile aspects', () => {
  for (const bounds of fitBounds) {
    for (const aspect of [1440 / 1000, 390 / 844, 4, 0.25]) {
      const value = cameraWidget(bounds, aspect);
      value.resetCamera();
      assertFits(value);
      assert.equal(value._panX, 0);
      assert.equal(value._panY, 0);
    }
  }
});

it('perspective corner fitting handles different camera orientations and camera zoom', () => {
  for (const bounds of fitBounds) {
    for (const [pitch, yaw] of [[0, 0], [-75, 23], [-90, 90], [30, -45]]) {
      const value = cameraWidget(bounds, 390 / 844);
      value._rotation.setFromAxisAngle(new THREE.Vector3(1, 0, 0), pitch * Math.PI / 180);
      value._rotation.multiply(new THREE.Quaternion().setFromAxisAngle(
          new THREE.Vector3(0, 0, 1), yaw * Math.PI / 180));
      value._camera.zoom = 1.5;
      value._panX = 0;
      value._panY = 0;
      value._distance = value._fitCameraDistance();
      value._updateCamera();
      assertFits(value);
    }
  }
});

it('initial scene build and reset fit actual rendered bounds including stacking', () => {
  const value = cameraWidget(fitBounds[0], 390 / 844);
  value._app.getDbuPerMicron = () => 1;
  value.buildScene({chiplets: [
    {name: 'a', x: -500, y: 20, z: 30, width: 2000, height: 50, thickness: 60},
    {name: 'b', x: -400, y: 20, z: 30, width: 1000, height: 50, thickness: 90},
  ]});
  assert.ok(value._chipletStackOffsets.some(offset => offset > 0));
  assertFits(value);
  value._distance *= 0.2;
  value._panX = 100;
  value._panY = -200;
  value._rotation.identity();
  value.resetCamera();
  assertFits(value);
  assert.equal(value._panX, 0);
  assert.equal(value._panY, 0);
  value._clearScene();
  value._edgeMaterial.dispose();
});

it('resizing a fitted view between desktop and mobile keeps every corner in frame', () => {
  for (const bounds of fitBounds) {
    const value = cameraWidget(bounds, 1440 / 1000);
    let size = {width: 1440, height: 1000};
    value._canvasContainer = {getBoundingClientRect: () => size};
    value._renderer = {setSize() {}};
    value.resetCamera();
    const desktopDistance = value._distance;
    for (size of [{width: 390, height: 844}, {width: 1440, height: 1000}]) {
      value.onResize();
      assertFits(value);
      assert.equal(value._camera.aspect, size.width / size.height);
    }
    assert.ok(Math.abs(value._distance - desktopDistance) < desktopDistance * 1e-12);
  }
});

it('resize preserves the user rotation, pan, and zoom relative to a fitted view', () => {
  const value = cameraWidget(fitBounds[0], 1440 / 1000);
  let size = {width: 390, height: 844};
  let renderedSize;
  value._canvasContainer = {getBoundingClientRect: () => size};
  value._renderer = {setSize: (...dimensions) => { renderedSize = dimensions; }};
  value._rotation.setFromAxisAngle(new THREE.Vector3(1, 0, 0), -Math.PI / 3);
  const rotation = value._rotation.toArray();
  value._panX = 7;
  value._panY = -13;
  value._distance = value._fitCameraDistance() * 0.6;
  value.onResize();
  assert.ok(Math.abs(value._distance / value._fitCameraDistance() - 0.6) < 1e-12);
  assert.deepEqual(value._rotation.toArray(), rotation);
  assert.equal(value._panX, 7);
  assert.equal(value._panY, -13);
  assert.deepEqual(renderedSize, [390, 844]);
  const distance = value._distance;
  const aspect = value._camera.aspect;
  size = {width: 0, height: 844};
  value.onResize();
  assert.equal(value._distance, distance);
  assert.equal(value._camera.aspect, aspect);
});
