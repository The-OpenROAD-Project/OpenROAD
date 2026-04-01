// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// DBU <-> Leaflet coordinate conversion.
//
// The tile renderer uses two Y-flips (tile index + pixel row), so the
// correct mapping is:
//   lat = (dbu_y - originY - maxDXDY) * scale
//   lng = (dbu_x - originX) * scale
//
// `scale`   = tileSize / max(designWidth, designHeight)  (pixels-per-DBU)
// `maxDXDY` = max(designWidth, designHeight) in DBU
// `originX` = bounds.xMin() in DBU (tile grid origin)
// `originY` = bounds.yMin() in DBU (tile grid origin)

export function dbuToLatLng(dbuX, dbuY, scale, maxDXDY, originX = 0, originY = 0) {
    return [(dbuY - originY - maxDXDY) * scale, (dbuX - originX) * scale];
}

export function dbuRectToBounds(x1, y1, x2, y2, scale, maxDXDY, originX = 0, originY = 0) {
    return [
        [(y1 - originY - maxDXDY) * scale, (x1 - originX) * scale],
        [(y2 - originY - maxDXDY) * scale, (x2 - originX) * scale],
    ];
}

export function latLngToDbu(lat, lng, scale, maxDXDY, originX = 0, originY = 0) {
    return {
        dbuX: Math.round(originX + lng / scale),
        dbuY: Math.round(originY + maxDXDY + lat / scale),
    };
}
