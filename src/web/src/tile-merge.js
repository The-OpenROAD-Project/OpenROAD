// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Client-side tile merging: collapse many single-layer tile panes into N
// composited panes, chosen from a decoded-image-memory budget.
//
// WHY
//
// The viewer keeps one Leaflet pane per (tech layer x chiplet instance).  On a
// multi-die design that is ~97 panes, each holding a full grid of tiles, each
// tile decoding to (256*dpr)^2 * 4 bytes — 582 MB at dpr 1 in a 1240x999
// window.  Measured on that machine, Chrome starts discarding decoded images
// somewhere around 458 MB, and the discarded regions paint white and stay white
// until something forces a full invalidation.  Established by measurement: the
// symptom is monotonic in decoded image bytes and indifferent to how they are
// distributed — at a matched 516 vs 517 MB, 86 and 97 content-bearing panes
// fail identically, which rules out pane count and stack depth.
//
// WHY THE MERGE IS ON THE CLIENT
//
// The client already knows the exact stacking order and per-pane opacity, so
// merging here reproduces precisely what the browser does today — one
// rasterization instead of K live layers — with no change to the renderer, no
// new wire format, and no decision about how chiplets should be ordered
// relative to each other.  It also keeps the server's per-layer tile cache
// intact, which a server-side composite would coarsen.
//
// The mechanism that makes it work is `ImageBitmap.close()`: an <img> gives no
// control over when its decoded bitmap is released, which is the whole problem.
// createImageBitmap + close() makes the lifetime explicit — decode K sources,
// draw them, release them immediately, and keep only the merged result.
//
// What it does NOT buy: bandwidth.  The same K images are still fetched per
// tile.  Only a server-side composite fixes that.

// ─── Opacity ────────────────────────────────────────────────────────────────
//
// Each layer's opacity is applied by Leaflet to the layer's CONTAINER div, not
// to the individual tile images: GridLayer._updateOpacity() calls
// setOpacity(this._container, this.options.opacity).  Every layer is created at
// 1 — layer transparency lives in the tile pixels themselves, painted with the
// palette alpha the Qt GUI uses — but the merge still carries opacity per item
// rather than assuming one value, so a pane that does want partial opacity
// composites correctly.
//
// Group opacity on a container is NOT generally the same thing as per-image
// alpha: a group is rasterized at full alpha and then blended once, so anywhere
// two images inside the group overlap, group opacity blends the overlap once
// while per-image alpha blends it twice.  It is safe here only because a
// layer's own tiles tile the plane without overlapping, which makes the two
// equivalent for a single layer — and that equivalence is what lets this module
// apply each source's opacity per-image with globalAlpha.
//
// (One transient exception: mid zoom-animation Leaflet keeps the outgoing zoom
// level's tiles alongside the incoming ones inside the same container, and those
// DO overlap.  That is pre-existing behaviour at the layer level and applies
// equally to a merged pane, so it is not made worse here.)
//
// The trap: the merged result is itself a tile inside a pane.  If that pane
// carried a source's opacity too, every source would be multiplied by it twice
// — 0.7 becomes 0.49 — and the view would wash out.  The per-layer opacities
// live INSIDE the canvas, so the pane holding it must be fully opaque.
export const MERGED_PANE_OPACITY = 1;

// Build the Leaflet options for a merged pane, so a caller cannot forget the
// above and silently double-apply opacity.
export function mergedPaneOptions({ zIndex = 0, tileSize } = {}) {
    const opts = { opacity: MERGED_PANE_OPACITY, zIndex };
    if (tileSize !== undefined) {
        opts.tileSize = tileSize;
    }
    return opts;
}

// Bytes of decoded bitmap for one tile.  Straight RGBA, which is what the
// browser holds regardless of the PNG's own bit depth — an 8-bit palette PNG
// still decodes to 32 bits per pixel, so compressing harder saves nothing here.
export const BYTES_PER_PIXEL = 4;

// Default budget for decoded tile images. Chosen from the measurement, not from
// taste: ~458 MB was where the failure started on the machine that was profiled,
// 462 MB already showed flicker, and 456 MB was clean. 350 MB leaves ~23% margin
// under that, which matters because the real ceiling varies with the machine, the
// Chrome build and whatever else the renderer is holding.
//
// Deliberately not derived from `navigator.deviceMemory`: no API reports the
// image-cache budget, and the failure cannot be probed for without triggering
// it. A fixed conservative cap that is *enforced* beats a discovered number that
// might be wrong. Callers may scale this if they have better information.
export const DEFAULT_BUDGET_BYTES = 350 * 1024 * 1024;

export function tileBytes(tileSizePx, dpr = 1) {
    const side = Math.round(tileSizePx * Math.max(0, dpr));
    return side * side * BYTES_PER_PIXEL;
}

// Tiles one pane holds for a viewport of this size.
//
// Leaflet covers the viewport and keeps a ring of tiles outside it, so this is
// (cols+1) x (rows+1) — deliberately an estimate, because the real count is only
// knowable once tiles exist and the budget has to be decided before then.  It
// tracks window area, which is the factor that would otherwise make any fixed
// group count fail on a maximised 4K window.
export function estimateTilesPerPane(width, height, tileSizePx = 256) {
    if (!(width > 0) || !(height > 0) || !(tileSizePx > 0)) {
        return 1;
    }
    return (Math.ceil(width / tileSizePx) + 1)
           * (Math.ceil(height / tileSizePx) + 1);
}

// Viewport to size the budget against, with fallbacks.
//
// A container that is hidden or not yet laid out reports clientWidth 0, and a
// zero viewport is the dangerous direction: estimateTilesPerPane returns 1,
// computeGroupCount then divides the whole budget by one tile and comes back
// with groupCount == paneCount — one pane per layer, no merging at all, and the
// log cheerfully reports "merged 94 layers into 94 panes".  The failure this
// exists to prevent then happens as soon as the panel is shown.
//
// Falling back to the window over-estimates (the map is a panel inside a
// layout, not the whole window), and over-estimating is safe: more tiles per
// pane means fewer panes, which is the conservative side of the budget.
export function measureViewport(container, win) {
    const w = win || (typeof window !== 'undefined' ? window : null);
    const width = (container && container.clientWidth > 0)
        ? container.clientWidth
        : ((w && w.innerWidth > 0) ? w.innerWidth : 1024);
    const height = (container && container.clientHeight > 0)
        ? container.clientHeight
        : ((w && w.innerHeight > 0) ? w.innerHeight : 768);
    return { width, height };
}

// Set a draw item's visibility, returning whether it actually changed.
//
// The return value is the point.  The layer tree's onChange walks EVERY node
// and asserts the desired state on each one, so a caller that marks its pane
// dirty unconditionally re-requests every tile in every pane on any checkbox
// click — roughly 2300 requests on the 97-layer design that motivated this.
// The per-layer panes never had that problem because Leaflet's addTo() is a
// no-op for a layer already on the map; this restores the same property.
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

// Panes that stay unmerged and so are not counted by the grouping, but do hold
// a full grid of tiles each: _instances, _pins, and the always-on highlight
// overlay.  (_modules and the heatmap are only mounted when enabled, so they are
// not reserved for; they will push the total up when switched on.)
//
// At dpr 1 these are ~6 MB each and hardly matter.  At dpr 3 a tile is 2.25 MB,
// so a pane is ~54 MB and the three together are ~162 MB — enough that a budget
// which ignored them would report a comfortable fit while the real total sat
// above the ceiling.
export const UNMERGED_PANE_COUNT = 3;

// Budget left for the merged panes once the unmerged ones are charged for.
//
// Never returns less than one group's worth: the viewer has to render
// something, and a budget so small that no group fits would be a blank map
// rather than a slow one.
export function reserveForUnmergedPanes(budgetBytes, tilesPerPane,
                                        bytesPerTile,
                                        unmergedPanes = UNMERGED_PANE_COUNT) {
    const perPane = tilesPerPane * bytesPerTile;
    if (!(perPane > 0)) {
        return budgetBytes;
    }
    return Math.max(perPane, budgetBytes - unmergedPanes * perPane);
}

// How many merged panes fit in the budget.
//
// `tilesPerPane` must come from what Leaflet actually holds, not from the
// viewport size, because it includes the retained ring outside the view and
// grows with window area — the factor that would otherwise make any fixed N
// fail on a maximised 4K window (97 panes at 2560x1440 is ~1.9 GB).
export function computeGroupCount({
    budgetBytes = DEFAULT_BUDGET_BYTES,
    tilesPerPane,
    bytesPerTile,
    paneCount,
} = {}) {
    if (!(tilesPerPane > 0) || !(bytesPerTile > 0) || !(paneCount > 0)) {
        return 1;
    }
    const perGroup = tilesPerPane * bytesPerTile;
    // Never more groups than there are panes to put in them: merging cannot
    // help past one pane per group, and asking for more would just recreate the
    // situation this exists to avoid.
    return Math.max(1, Math.min(paneCount, Math.floor(budgetBytes / perGroup)));
}

// Split z-ordered draw items into `n` groups.
//
// Groups MUST be contiguous runs of the z-order. Each group is rasterized into
// a single image, and two images can only be stacked to reproduce the original
// if neither interleaves with the other: given z-order [1,2,3,4], the grouping
// {1,3},{2,4} has no stacking that yields the original, whereas {1,2},{3,4}
// does. So this slices the ordered list rather than distributing round-robin,
// even though round-robin would balance sizes better.
//
// `items` must already be sorted by ascending z-index (bottom-most first).
export function partitionIntoGroups(items, n) {
    if (!Array.isArray(items) || items.length === 0) {
        return [];
    }
    const groups = Math.max(1, Math.min(Math.floor(n) || 1, items.length));
    const base = Math.floor(items.length / groups);
    let remainder = items.length % groups;
    const out = [];
    let at = 0;
    for (let g = 0; g < groups; g++) {
        let size = base;
        if (remainder > 0) {
            size++;
            remainder--;
        }
        out.push(items.slice(at, at + size));
        at += size;
    }
    return out;
}

// Draw `sources` onto a 2D context in order, each at its own opacity.
//
// Straight src-over at the given alpha, which is what stacking the panes does
// today. Not bit-identical to CSS compositing — colour-space and rounding
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
// Why grouping is exact at all: src-over is associative, so
// (A over B) over C == A over (B over C).  Rasterizing a contiguous run into one
// image and compositing that image is therefore identical to compositing the run
// member by member — PROVIDED the intermediate keeps its alpha (a transparent
// canvas does) and is composited at alpha 1 (hence MERGED_PANE_OPACITY).
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
// Incremental: the tile paints as soon as anything arrives rather than waiting
// for the slowest of its K layers, and keeps repainting as the rest land.  A
// request that never settles therefore costs one missing layer instead of a
// blank tile — with the per-layer panes a stuck request blanked one layer, and
// waiting for the whole group would have made that a whole group.
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
    onFirstDraw,
} = {}) {
    const list = Array.isArray(items) ? items : [];
    const stats = { requested: list.length, arrived: 0, decoded: 0, drawn: 0,
                    paints: 0, stale: false };
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
        if (!painted) {
            painted = true;
            // Leaflet hides a tile until done() marks it loaded
            // (.leaflet-tile { visibility: hidden }), so without this the
            // incremental paints would not be visible at all.
            if (onFirstDraw) {
                onFirstDraw();
            }
        }
    }

    try {
        await Promise.all(list.map(async (item, i) => {
            let payload = null;
            try {
                payload = await request(item);
            } catch (e) {
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
            paint();
        }));

        // Nothing arrived at all — paint the empty composite anyway, so the
        // tile resolves as blank rather than staying invisible forever.
        if (!painted) {
            paint();
        }
    } finally {
        // Held until the tile is complete rather than released per draw: a
        // later arrival belongs UNDERNEATH layers already drawn, so the canvas
        // is recomposited from scratch each time and every source has to still
        // be available.  Peak is the same K bitmaps the batch version held.
        release(images);
    }
    return stats;
}

// Summary of what a grouping costs, for logging and for the HUD.
export function describePlan({ paneCount, groupCount, tilesPerPane,
                               bytesPerTile }) {
    const mb = (bytes) => Math.round(bytes / (1024 * 1024));
    const before = paneCount * tilesPerPane * bytesPerTile;
    const after = groupCount * tilesPerPane * bytesPerTile;
    return {
        paneCount,
        groupCount,
        tilesPerPane,
        beforeMB: mb(before),
        afterMB: mb(after),
        // Peak extra while a group is being merged: its sources are decoded
        // together, then closed. Worth reporting because a very small
        // groupCount means very large groups, and the transient is what would
        // bite if the merge held every source at once.
        transientMB: mb(Math.ceil(paneCount / Math.max(1, groupCount))
                        * bytesPerTile),
    };
}
