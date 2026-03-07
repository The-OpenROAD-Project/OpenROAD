// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// DBU <-> Leaflet coordinate conversion.
//
// The tile renderer uses two Y-flips (tile index + pixel row), so the
// correct mapping is:
//   lat = (dbu_y - maxDXDY) * scale
//   lng = dbu_x * scale
//
// `scale`   = tileSize / max(designWidth, designHeight)  (pixels-per-DBU)
// `maxDXDY` = max(designWidth, designHeight) in DBU

export function dbuToLatLng(dbuX, dbuY, scale, maxDXDY) {
    return [(dbuY - maxDXDY) * scale, dbuX * scale];
}

export function dbuRectToBounds(x1, y1, x2, y2, scale, maxDXDY) {
    return [
        [(y1 - maxDXDY) * scale, x1 * scale],
        [(y2 - maxDXDY) * scale, x2 * scale],
    ];
}

export function latLngToDbu(lat, lng, scale, maxDXDY) {
    return {
        dbuX: Math.round(lng / scale),
        dbuY: Math.round(maxDXDY + lat / scale),
    };
}
