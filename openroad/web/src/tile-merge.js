// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Client-side tile merging: composite the visible layout layers of one tile
// into a single image, so the view holds and draws one texture per tile
// instead of one per layer.  The server keeps one cached tile per layer, which
// a layer toggle reuses.
//
// The sources are ImageBitmaps, closed as soon as the tile is complete: an
// <img> gives no control over when its decoded bitmap is released.

// Set a draw item's visibility, returning whether it actually changed.
//
// The return value is the point.  The layer tree's onChange walks EVERY node
// and asserts the desired state on each one, so a caller that marks its tiles
// dirty unconditionally re-requests every tile on any checkbox click.
export function setItemVisible(item, visible) {
    if (!item) {
        return false;
    }
    const want = !!visible;
    if (!!item.visible === want) {
        return false;
    }
    item.visible = want;
    return true;
}

// ─── Opacity ────────────────────────────────────────────────────────────────
//
// Each source carries its own opacity, applied per image with globalAlpha.  A
// layer's own tiles never overlap, so that equals opacity on the whole layer.
// The merged tile already holds those opacities and must be drawn at 1, or
// every source would be multiplied twice.

// Draw `sources` onto a 2D context in order, each at its own opacity.
//
// Straight src-over at the given alpha, which is what stacking the layers
// would do. Not bit-identical to GPU compositing — color-space and rounding
// details differ — so assert on visual equivalence, never on byte equality.
//
// `sources` is [{ image, opacity }], bottom-most first. A null image is skipped
// rather than throwing: one layer failing to decode must not lose the whole
// group, which would turn a single missing layer into a blank tile.
export function mergeOntoContext(ctx, sources, size) {
    ctx.clearRect(0, 0, size, size);
    let drawn = 0;
    for (const { image, opacity = 1 } of sources) {
        if (!image || !(opacity > 0)) {
            continue;
        }
        ctx.globalAlpha = Math.min(1, opacity);
        ctx.drawImage(image, 0, 0, size, size);
        drawn++;
    }
    ctx.globalAlpha = 1;
    return drawn;
}

// ─── Reference compositor ───────────────────────────────────────────────────
//
// Why merging is exact at all: src-over is associative, so
// (A over B) over C == A over (B over C).  Rasterizing a contiguous run into one
// image and compositing that image is therefore identical to compositing the run
// member by member — PROVIDED the intermediate keeps its alpha (a transparent
// canvas does) and is composited at alpha 1.
//
// These helpers exist so that invariant is checked with numbers instead of
// asserted in a comment.  They are not used by the merge itself, which delegates
// to the canvas; they model what the canvas and the browser both do.
//
// Straight (non-premultiplied) RGBA, channels 0..255, matching the convention in
// tile_generator.cpp's blendPixel.
export function blendOver(dst, src, opacity = 1) {
    const sa = (src[3] / 255) * Math.max(0, Math.min(1, opacity));
    const da = dst[3] / 255;
    const oa = sa + da * (1 - sa);
    if (oa <= 0) {
        return [0, 0, 0, 0];
    }
    const out = [0, 0, 0, 0];
    for (let c = 0; c < 3; c++) {
        out[c] = (src[c] * sa + dst[c] * da * (1 - sa)) / oa;
    }
    out[3] = oa * 255;
    return out;
}

// Composite a bottom-most-first stack of { rgba, opacity } onto `base`.
export function compositeStack(sources, base = [0, 0, 0, 0]) {
    let out = base.slice();
    for (const s of sources || []) {
        if (!s || !s.rgba) {
            continue;
        }
        out = blendOver(out, s.rgba, s.opacity === undefined ? 1 : s.opacity);
    }
    return out;
}

// Release decoded bitmaps immediately instead of waiting for GC to notice.
// The explicit release is the entire reason for using ImageBitmap over <img>,
// so a leak here defeats the purpose. Tolerates plain images (no close()).
export function closeAll(images) {
    for (const image of images || []) {
        if (image && typeof image.close === 'function') {
            image.close();
        }
    }
}

// ─── Rendering one merged tile ──────────────────────────────────────────────
//
// Every browser dependency is injected: `request` fetches one item's tile,
// `decode` turns a payload into something drawable and `draw` composites.
// `items` is never empty; the source skips such a tile.
//
// The requests go out together, and every arrival recomposites the list in
// order, so the stacking does not depend on the order the replies land in.
//
// Incremental (the default) paints as soon as anything arrives, so a request
// that never settles costs one layer, not the tile.  `incremental: false`
// (Options > "Double buffering") paints once, after every request settled.
//
// Every decode is closed in a finally, so `request` must always settle: a
// cancelled request rejects (WebSocketManager.cancel).  An item that fails to
// arrive or decode is skipped and the rest still paints.
export async function renderMergedTile({
    items,
    request,
    decode,
    draw,
    isStale = () => false,
    incremental = true,
}) {
    const stats = { requested: items.length, arrived: 0, decoded: 0, drawn: 0,
                    failed: 0, stale: false };

    // A slot that has not arrived yet is drawn as nothing.  That is exact, not
    // an approximation: transparent is the identity for src-over, so
    // compositing the arrived subset in list order gives precisely the same
    // pixels as the full composite would where those layers are absent.  It is
    // what lets the tile paint before every layer is in without any risk of
    // getting the stacking wrong — the list is still walked in order every
    // time, so there is no prefix pointer or gap handling to get wrong.
    const images = new Array(items.length).fill(null);

    function paint() {
        if (isStale()) {
            stats.stale = true;
            return;
        }
        stats.drawn = draw(items.map((item, i) => ({
            image: images[i],
            opacity: item.opacity,
        }))) || 0;
    }

    try {
        await Promise.all(items.map(async (item, i) => {
            let payload = null;
            try {
                payload = await request(item);
            } catch (e) {
                stats.failed++;
                return;  // cancelled or dropped: leave the slot transparent
            }
            if (payload == null) {
                return;
            }
            stats.arrived++;
            let image = null;
            try {
                image = await decode(payload);
            } catch (e) {
                return;  // one undecodable tile must not lose the group
            }
            if (!image) {
                return;
            }
            // Stored before the staleness check so the release below frees it
            // even when this render has been superseded.
            images[i] = image;
            stats.decoded++;
            if (incremental) {
                paint();
            }
        }));

        // Buffered: the one paint, once every request settled.
        if (!incremental) {
            paint();
        }
    } finally {
        // Held until the tile is complete rather than released per draw: a
        // later arrival belongs UNDERNEATH layers already drawn, so the canvas
        // is recomposited from scratch each time and every source has to still
        // be available.
        closeAll(images);
    }
    return stats;
}
