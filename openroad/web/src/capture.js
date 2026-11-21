// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// WYSIWYG layout capture.  Copies the view's canvas (tiles, overlays,
// heatmap, selection, rulers) and the ruler distance labels into a PNG and
// downloads it — mirroring the Qt GUI's "Save image", which saves the
// displayed scene.  The PNG is in CSS pixels.
//
// entire=true first fits the whole design (no animation), waits for every
// layer's tiles to settle, then captures, and restores the previous view.

function downloadBlob(blob, filename) {
    if (!blob) {
        return;
    }
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    a.style.display = 'none';
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    setTimeout(() => URL.revokeObjectURL(url), 0);
}

// Rasterize the ruler distance labels, which are DOM markers over the canvas.
//
// Painted directly onto the canvas rather than serialized through an <img>
// data URL: a detached document does not inherit the app's stylesheet, so a
// foreignObject copy of the label would come out unstyled (no background,
// default font).  Reading the computed style and drawing the box and text
// keeps it looking like what is on screen.
function drawHtmlLabels(app, ctx, mapRect) {
    const labels = app.view.getContainer()
        .querySelectorAll('.or-view-markers .ruler-label');
    labels.forEach((el) => {
        const r = el.getBoundingClientRect();
        if (r.width <= 0 || r.height <= 0) {
            return;
        }
        const text = el.textContent;
        if (!text) {
            return;
        }
        const cs = getComputedStyle(el);
        const x = r.left - mapRect.left;
        const y = r.top - mapRect.top;
        ctx.save();
        ctx.fillStyle = cs.backgroundColor || 'rgba(0, 0, 0, 0.75)';
        // Round the corners when the canvas supports it; a square box is a
        // better failure than no label at all.
        const radius = Math.min(parseFloat(cs.borderTopLeftRadius) || 0,
                                r.height / 2);
        if (radius > 0 && typeof ctx.roundRect === 'function') {
            ctx.beginPath();
            ctx.roundRect(x, y, r.width, r.height, radius);
            ctx.fill();
        } else {
            ctx.fillRect(x, y, r.width, r.height);
        }
        ctx.fillStyle = cs.color || '#eee';
        ctx.font = cs.font || `${cs.fontSize || '12px'} ${cs.fontFamily || 'monospace'}`;
        ctx.textAlign = 'left';
        ctx.textBaseline = 'middle';
        ctx.fillText(text, x + (parseFloat(cs.paddingLeft) || 0),
                     y + r.height / 2);
        ctx.restore();
    });
}

async function renderToBlob(app) {
    const container = app.view.getContainer();
    const mapRect = container.getBoundingClientRect();
    const w = Math.max(1, Math.round(mapRect.width));
    const h = Math.max(1, Math.round(mapRect.height));
    const canvas = document.createElement('canvas');
    canvas.width = w;
    canvas.height = h;
    const ctx = canvas.getContext('2d');
    // Background matches the viewer (--bg-map); fall back to #000 when the
    // computed color is empty or fully transparent (both would export a
    // transparent PNG, hiding light elements on white viewers).
    const bg = getComputedStyle(container).backgroundColor;
    ctx.fillStyle = (bg && bg !== 'transparent' && bg !== 'rgba(0, 0, 0, 0)')
        ? bg : '#000';
    ctx.fillRect(0, 0, w, h);
    const scene = await app.view.captureCanvas();
    if (scene) {
        const r = scene.rect;
        try {
            ctx.drawImage(scene.image, r.left - mapRect.left,
                          r.top - mapRect.top, r.width, r.height);
        } catch (err) {
            // A canvas that cannot be drawn yet leaves the background only.
        }
    }
    // Last, so the labels sit above the ruler lines they annotate.
    drawHtmlLabels(app, ctx, mapRect);
    return new Promise((resolve) => canvas.toBlob(resolve, 'image/png'));
}

// Public entry point.  entire=false captures the current viewport;
// entire=true fits the whole design first, waits for the newly requested tiles
// to settle, captures, then restores the previous view.  Both are WYSIWYG:
// base layers + heatmap + highlights/selection + rulers, exactly as shown.
export async function captureLayout(app, { entire = false } = {}) {
    if (!app.view) {
        return;
    }
    // Entire needs whole-design bounds; without them the capture would just be
    // the current viewport, so save it (honestly) as the visible layout.
    if (!entire || !app.fitRect) {
        downloadBlob(await renderToBlob(app), 'layout_visible.png');
        return;
    }

    const prevCenter = app.view.getCenter();
    const prevZoom = app.view.getZoom();
    try {
        app.view.fitWorldRect(app.fitRect, { animate: false });
        await app.view.whenTilesSettled();
        downloadBlob(await renderToBlob(app), 'layout_entire.png');
    } finally {
        app.view.setView(prevCenter, prevZoom, { animate: false });
    }
}
