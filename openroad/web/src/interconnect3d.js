// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Bump rows: [chiplet index, xmin, ymin, xmax, ymax, surface z], in DBU.
// These are footprints, not fabricated bump heights. Net links are flight
// lines between real terminals, not a claim about physical routing topology.
export function createInterconnectMeshes(data, offsets, dbu, zScale, THREE) {
  const rows = data.bumps || [];
  let bumps = null;
  let links = null;
  if (rows.length) {
    const geometry = new THREE.PlaneGeometry(1, 1);
    const material = new THREE.MeshBasicMaterial({
      color: 0xffd54f,
      side: THREE.DoubleSide,
      polygonOffset: true,
      polygonOffsetFactor: -1,
      polygonOffsetUnits: -1,
    });
    bumps = new THREE.InstancedMesh(geometry, material, rows.length);
    const matrix = new THREE.Matrix4();
    rows.forEach(([chip, x1, y1, x2, y2, z], i) => {
      matrix.makeScale((x2 - x1) / dbu, (y2 - y1) / dbu, 1);
      matrix.setPosition((x1 + x2) / (2 * dbu),
                         (y1 + y2) / (2 * dbu),
                         z * zScale / dbu + (offsets[chip] || 0));
      bumps.setMatrixAt(i, matrix);
    });
    bumps.instanceMatrix.needsUpdate = true;
    bumps.userData.interconnect = 'bumps';
  }

  const nets = data.nets || [];
  const segmentCount = nets.reduce(
      (sum, net) => sum + Math.max(0, net.bumps.length - 1), 0);
  if (segmentCount) {
    const positions = new Float32Array(segmentCount * 6);
    let at = 0;
    const append = (index) => {
      const [chip, x1, y1, x2, y2, z] = rows[index];
      positions[at++] = (x1 + x2) / (2 * dbu);
      positions[at++] = (y1 + y2) / (2 * dbu);
      positions[at++] = z * zScale / dbu + (offsets[chip] || 0);
    };
    for (const net of nets) {
      // A star is only a connectivity visualization, not extracted routing.
      for (let i = 1; i < net.bumps.length; ++i) {
        append(net.bumps[0]);
        append(net.bumps[i]);
      }
    }
    const geometry = new THREE.BufferGeometry();
    geometry.setAttribute('position',
                          new THREE.Float32BufferAttribute(positions, 3));
    links = new THREE.LineSegments(geometry, new THREE.LineBasicMaterial({
      color: 0x00bcd4,
      transparent: true,
      opacity: 0.85,
    }));
    links.userData.interconnect = 'links';
  }
  return {bumps, links};
}
