// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Client-side tile merging: composite the visible layout layers of one tile
// into a single image, so the view holds and draws one texture per tile
// instead of one per layer.
//
// WHY THE MERGE IS ON THE CLIENT
//
// The client already knows the exact stacking order and per-layer opacity, so
// merging here is one rasterization instead of K live layers, and the server
// keeps one cached tile per layer, which a layer toggle reuses.
//
// The mechanism that makes it work is `ImageBitmap.close()`: an <img> gives no
// control over when its decoded bitmap is released, which is the whole problem.
// createImageBitmap + close() makes the lifetime explicit — decode K sources,
// draw them, release them immediately, and keep only the merged result.
//
// What it does NOT buy: bandwidth.  The same K images are still fetched per
// tile.

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
// would do. Not bit-identical to GPU compositing — colour-space and rounding
// details differ — so assert on visual equivalence, never on byte equality.
//
// `sources` is [{ image, opacity }], bottom-most first. A null image is skipped
// rather than throwing: one layer failing to decode must not lose the whole
// group, which would turn a single missing layer into a blank tile.
export function mergeOntoContext(ctx, sources, size) {
    if (!ctx || !(size > 0)) {
        return 0;
    }
    ctx.clearRect(0, 0, size, size);
    let drawn = 0;
    for (const source of sources || []) {
        const image = source ? source.image : null;
        if (!image) {
            continue;
        }
        const opacity = source.opacity === undefined ? 1 : source.opacity;
        if (!(opacity > 0)) {
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
// The orchestration, with every browser dependency injected so it can be tested
// without a DOM: `request` fetches one item's tile, `decode` turns a payload
// into something drawable, `draw` composites, `release` frees the decodes.
//
// Ordering: the K requests go out together, because they are independent and
// serialising them would multiply latency by K.  The canvas is recomposited
// from scratch on every arrival, always walking the item list in order, so the
// stacking cannot depend on the order responses happen to land in.
//
// Incremental (the default): the tile paints as soon as anything arrives rather
// than waiting for the slowest of its K layers, and keeps repainting as the rest
// land.  A request that never settles therefore costs one missing layer instead
// of a blank tile; waiting for the whole group would blank the whole tile.
//
// `incremental: false` (Options > "Double buffering") paints once, after every
// request has settled, so a refreshed tile swaps whole, not layer by layer.
//
// Lifetime: every decoded image is released in a finally, including on a draw
// failure or a stale abort.  Explicit release is the entire reason for
// ImageBitmap over <img>; leaking here would reintroduce exactly the unbounded
// decoded-image growth this exists to bound.  That rests on `request` always
// settling: a cancelled request has to reject rather than stay pending, or the
// finally never runs and the group's decodes are stranded (see
// WebSocketManager.cancel, which rejects both the queued and the sent case).
//
// Partial failure is survivable by design: one item that fails to arrive or
// decode is skipped and the rest of the group still paints.  Blanking the whole
// tile because one layer failed would turn a single dropped request into the
// symptom being fixed.
export async function renderMergedTile({
    items,
    request,
    decode,
    draw,
    release = closeAll,
    isStale = () => false,
    incremental = true,
} = {}) {
    const list = Array.isArray(items) ? items : [];
    const stats = { requested: list.length, arrived: 0, decoded: 0, drawn: 0,
                    paints: 0, failed: 0, stale: false };
    if (list.length === 0) {
        // An empty group still has to paint: the canvas is reused across
        // refreshes, so skipping the draw would leave the previous content.
        // Guarded only so a no-argument call is harmless — a real caller that
        // forgets `draw` with a non-empty group still fails loudly below,
        // which is what you want for a missing required dependency.
        if (typeof draw === 'function') {
            draw([]);
        }
        return stats;
    }

    // A slot that has not arrived yet is drawn as nothing.  That is exact, not
    // an approximation: transparent is the identity for src-over, so
    // compositing the arrived subset in list order gives precisely the same
    // pixels as the full composite would where those layers are absent.  It is
    // what lets the tile paint before every layer is in without any risk of
    // getting the stacking wrong — the list is still walked in order every
    // time, so there is no prefix pointer or gap handling to get wrong.
    const images = new Array(list.length).fill(null);
    let painted = false;

    function paint() {
        if (isStale()) {
            stats.stale = true;
            return;
        }
        stats.drawn = draw(list.map((item, i) => ({
            image: images[i],
            opacity: item.opacity,
        }))) || 0;
        stats.paints++;
        painted = true;
    }

    try {
        await Promise.all(list.map(async (item, i) => {
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

        // The only paint when buffering; otherwise the case where nothing
        // arrived at all, so the tile resolves as blank rather than staying
        // invisible forever.
        if (!painted) {
            paint();
        }
    } finally {
        // Held until the tile is complete rather than released per draw: a
        // later arrival belongs UNDERNEATH layers already drawn, so the canvas
        // is recomposited from scratch each time and every source has to still
        // be available.
        release(images);
    }
    return stats;
}
