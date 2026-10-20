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
