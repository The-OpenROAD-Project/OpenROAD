// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// WYSIWYG layout capture.  Composites the currently rendered Leaflet layers
// (base tiles, the highlight/selection overlay, the active heatmap) plus the
// SVG overlays (rulers, selection outline, labels) into a PNG and downloads
// it — mirroring the Qt GUI's "Save image", which saves the displayed scene.
//
// All tiles are same-origin <img> (blob: URLs), so the canvas is not tainted
// and toBlob() works.  Tile positions are read from getBoundingClientRect so
// the composite is correct under fractional zoom / CSS transforms.
//
// entire=true first fits the whole design (no animation), waits for every
// layer's tiles to settle, then composites — capturing the full design exactly
// as the window shows it (layers + heatmap + highlights + rulers), and restores
// the previous view afterwards.

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

// A layer's effective opacity (default 1 when unset).
function layerOpacity(layer) {
    return (layer.options && layer.options.opacity != null)
        ? layer.options.opacity : 1;
}

// Draw one <img>/element at its on-screen position relative to the map. Skips
// an <img> that hasn't decoded yet, and swallows a not-yet-drawable element.
function drawElementAt(ctx, el, mapRect) {
    if (!el) {
        return;
    }
    if (el.tagName === 'IMG' && (!el.complete || !el.naturalWidth)) {
        return;  // still loading
    }
    const r = el.getBoundingClientRect();
    if (r.width <= 0 || r.height <= 0) {
        return;
    }
    try {
        ctx.drawImage(el, r.left - mapRect.left, r.top - mapRect.top,
                      r.width, r.height);
    } catch (err) {
        // Skip an element that can't be drawn yet.
    }
}

// Draw every loaded GridLayer <img> tile onto ctx, in z-index order, at its
// on-screen position relative to the map container.
function drawGridLayers(app, ctx, mapRect) {
    const layers = [];
    app.map.eachLayer((layer) => {
        if (layer instanceof L.GridLayer && layer._tiles) {
            layers.push(layer);
        }
    });
    layers.sort((a, b) =>
        ((a.options && a.options.zIndex) || 0)
        - ((b.options && b.options.zIndex) || 0));

    for (const layer of layers) {
        const opacity = layerOpacity(layer);
        if (opacity <= 0) {
            continue;
        }
        ctx.globalAlpha = opacity;
        for (const key of Object.keys(layer._tiles)) {
            const t = layer._tiles[key];
            if (!t || !t.current) {
                continue;
            }
            drawElementAt(ctx, t.el, mapRect);
        }
    }
    ctx.globalAlpha = 1;
}

// Rasterize the SVG overlay panes (rulers, selection outline, labels) on top.
// Async: each SVG is drawn through an <img> data URL.
function drawSvgOverlays(app, ctx, mapRect) {
    const svgs = app.map.getContainer().querySelectorAll('.leaflet-pane svg');
    const jobs = [];
    svgs.forEach((svg) => {
        const r = svg.getBoundingClientRect();
        if (r.width <= 0 || r.height <= 0) {
            return;
        }
        const clone = svg.cloneNode(true);
        clone.setAttribute('width', r.width);
        clone.setAttribute('height', r.height);
        const xml = new XMLSerializer().serializeToString(clone);
        const url = 'data:image/svg+xml;charset=utf-8,'
            + encodeURIComponent(xml);
        jobs.push(new Promise((resolve) => {
            const img = new Image();
            img.onload = () => {
                try {
                    ctx.drawImage(img, r.left - mapRect.left,
                                  r.top - mapRect.top, r.width, r.height);
                } catch (err) { /* ignore */ }
                resolve();
            };
            img.onerror = () => resolve();
            img.src = url;
        }));
    });
    return Promise.all(jobs);
}

// Count tiles that haven't finished loading across every GridLayer (base,
// overlay/highlights, heatmap, ...): a layer still marked _loading, or a
// current tile whose <img> hasn't decoded yet.  Zero => the scene is painted.
function tilesPending(app) {
    let pending = 0;
    app.map.eachLayer((layer) => {
        if (!(layer instanceof L.GridLayer) || !layer._tiles) {
            return;
        }
        if (layer._loading) {
            pending++;
        }
        for (const key of Object.keys(layer._tiles)) {
            const t = layer._tiles[key];
            const el = t && t.el;
            if (t && t.current && el && el.tagName === 'IMG'
                && (!el.complete || !el.naturalWidth)) {
                pending++;
            }
        }
    });
    return pending;
}

// Resolve once all layers' tiles have settled (stable across a couple of polls)
// or a timeout elapses.  Used after a fit so the capture waits for the newly
// requested tiles (base + overlay + heatmap) to arrive.
function whenTilesSettled(app, timeoutMs = 6000, intervalMs = 32) {
    return new Promise((resolve) => {
        const deadline = Date.now() + timeoutMs;
        let stable = 0;
        const check = () => {
            if (tilesPending(app) === 0) {
                if (++stable >= 2) {
                    resolve();
                    return;
                }
            } else {
                stable = 0;
            }
            if (Date.now() >= deadline) {
                resolve();
                return;
            }
            setTimeout(check, intervalMs);
        };
        setTimeout(check, intervalMs);
    });
}

// Draw single-image overlays (e.g. the timing-path L.ImageOverlay) at their
// on-screen position, so they land in the capture like the tile layers.
function drawImageOverlays(app, ctx, mapRect) {
    app.map.eachLayer((layer) => {
        if (!(layer instanceof L.ImageOverlay)) {
            return;
        }
        const opacity = layerOpacity(layer);
        if (opacity <= 0) {
            return;
        }
        ctx.globalAlpha = opacity;
        drawElementAt(ctx, layer._image, mapRect);
        ctx.globalAlpha = 1;
    });
}

async function renderToBlob(app) {
    const container = app.map.getContainer();
    const mapRect = container.getBoundingClientRect();
    const w = Math.max(1, Math.round(mapRect.width));
    const h = Math.max(1, Math.round(mapRect.height));
    const canvas = document.createElement('canvas');
    canvas.width = w;
    canvas.height = h;
    const ctx = canvas.getContext('2d');
    // Background matches the viewer (--bg-map); fall back to #111 when the
    // computed color is empty or fully transparent (both would export a
    // transparent PNG, hiding light elements on white viewers).
    const bg = getComputedStyle(container).backgroundColor;
    ctx.fillStyle = (bg && bg !== 'transparent' && bg !== 'rgba(0, 0, 0, 0)')
        ? bg : '#111';
    ctx.fillRect(0, 0, w, h);
    drawGridLayers(app, ctx, mapRect);
    drawImageOverlays(app, ctx, mapRect);
    await drawSvgOverlays(app, ctx, mapRect);
    return new Promise((resolve) => canvas.toBlob(resolve, 'image/png'));
}

// Public entry point.  entire=false composites the current viewport;
// entire=true fits the whole design first, waits for the newly requested tiles
// to settle, captures, then restores the previous view.  Both are WYSIWYG:
// base layers + heatmap + highlights/selection + rulers, exactly as shown.
export async function captureLayout(app, { entire = false } = {}) {
    if (!app.map) {
        return;
    }
    // Entire needs whole-design bounds; without them the capture would just be
    // the current viewport, so save it (honestly) as the visible layout.
    if (!entire || !app.fitBounds) {
        downloadBlob(await renderToBlob(app), 'layout_visible.png');
        return;
    }

    const prevCenter = app.map.getCenter();
    const prevZoom = app.map.getZoom();
    try {
        app.map.fitBounds(app.fitBounds, { animate: false });
        await whenTilesSettled(app);
        downloadBlob(await renderToBlob(app), 'layout_entire.png');
    } finally {
        app.map.setView(prevCenter, prevZoom, { animate: false });
    }
}
