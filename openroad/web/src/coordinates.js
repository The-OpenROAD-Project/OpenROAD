// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// DBU <-> layout-view world coordinates: world = (dbu - origin) * scale, y up,
// so the tile grid is the square [0, tileSize] on both axes.
//
// `scale`   = tileSize / max(designWidth, designHeight)  (pixels-per-DBU)
// `originX` = bounds.xMin() in DBU (tile grid origin)
// `originY` = bounds.yMin() in DBU (tile grid origin)

export function dbuToWorld(dbuX, dbuY, scale, originX = 0, originY = 0) {
    return [(dbuX - originX) * scale, (dbuY - originY) * scale];
}

export function worldToDbu(wx, wy, scale, originX = 0, originY = 0) {
    return {
        dbuX: Math.round(originX + wx / scale),
        dbuY: Math.round(originY + wy / scale),
    };
}

// World rect [minX, minY, maxX, maxY] of a DBU rect given by two corners.
export function dbuRectToWorldRect(x1, y1, x2, y2, scale, originX = 0, originY = 0) {
    const [ax, ay] = dbuToWorld(x1, y1, scale, originX, originY);
    const [bx, by] = dbuToWorld(x2, y2, scale, originX, originY);
    return [Math.min(ax, bx), Math.min(ay, by), Math.max(ax, bx), Math.max(ay, by)];
}
