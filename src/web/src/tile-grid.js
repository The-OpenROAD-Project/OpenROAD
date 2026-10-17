// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The tiles of one slot of the layout view: which to load for the view, which
// to cancel and which to drop.  Pure: loading goes through `source`, drawing
// is the view's business.
//
// `source.load({z, x, y}, {background, onPartial})` returns
// `{ promise, cancel, promote }`; the promise resolves to a drawable, to null
// for an empty tile, or to undefined to keep the last partial.
// `source.release(drawable)` frees one.

import { growRange, inRange, rangeTiles } from './view-math.js';

export function tileKey(z, x, y) {
    return `${z}/${x}/${y}`;
}

export class TileGrid {
    constructor({ source, onChange = () => {}, keepBuffer = 2 } = {}) {
        this.source = source;
        this.onChange = onChange;
        this.keepBuffer = keepBuffer;
        this.tiles = new Map();
        this.z = null;
        this.range = null;   // on screen
        this.ring = null;    // on screen plus the prefetch ring
        this.extra = [];     // other levels kept loaded: [{ z, range }]
        this._generation = 0;
        this._loads = 0;     // for debugSnapshot
        this._cancels = 0;
    }

    // Bring level z in line with the view: load what `ring` covers, on-screen
    // tiles first and the rest at background priority, and cancel what left.
    // `extra` levels load over their own ranges, in the background unless
    // they say `background: false`.  Loaded tiles in `hold`, what is on
    // screen now when `range` is where the view is heading, are kept.
    update({ z, range, ring = range, extra = [], hold = null }) {
        this.z = z;
        this.range = range;
        this.ring = ring;
        this.extra = extra.filter((e) => e.z !== z && e.range);
        const extraOf = new Map(this.extra.map((e) => [e.z, e]));
        for (const tile of [...this.tiles.values()]) {
            if (tile.z !== z) {
                const e = extraOf.get(tile.z);
                const r = e && e.range;
                // An extra level keeps a buffer like level z; any other level
                // is left to prune(), which knows what is drawn.
                const drop = r
                    ? !inRange(r, tile.x, tile.y)
                      && (tile.loading || !this._inKeepBuffer(tile, r))
                    : tile.loading && !tile.drawable;
                if (drop) {
                    this._drop(tile);
                } else if (r && this._stale(tile)
                           && inRange(r, tile.x, tile.y)) {
                    this._load(tile, e.background !== false);
                }
                continue;
            }
            // Kept from before a refresh: fetch again once it is in the ring.
            if (this._stale(tile) && inRange(ring, tile.x, tile.y)) {
                this._load(tile, !inRange(range, tile.x, tile.y));
            }
            if (inRange(ring, tile.x, tile.y)) {
                if (tile.loading && tile.background
                    && inRange(range, tile.x, tile.y)) {
                    tile.background = false;
                    tile.handle?.promote?.();
                }
                continue;
            }
            if (tile.loading || !(this._inKeepBuffer(tile)
                                  || (hold && inRange(hold, tile.x, tile.y)))) {
                this._drop(tile);
            }
        }
        // On-screen tiles go out first, nearest the centre first.
        const wanted = [...rangeTiles(range),
                        ...rangeTiles(ring).filter(
                            (t) => !inRange(range, t.x, t.y))];
        for (const { x, y } of wanted) {
            if (!this.tiles.has(tileKey(z, x, y))) {
                this._load(this._newTile(z, x, y),
                           !inRange(range, x, y));
            }
        }
        for (const e of this.extra) {
            for (const { x, y } of rangeTiles(e.range)) {
                if (!this.tiles.has(tileKey(e.z, x, y))) {
                    this._load(this._newTile(e.z, x, y),
                               e.background !== false);
                }
            }
        }
    }

    // Re-request the tiles of the ring and of the extra ranges in place: each
    // keeps what it shows until its replacement arrives.  The rest goes stale
    // and is fetched when it is back in range; other levels wait for prune().
    refresh() {
        this._generation++;
        const extraOf = new Map(this.extra.map((e) => [e.z, e]));
        for (const tile of [...this.tiles.values()]) {
            const e = extraOf.get(tile.z);
            const r = tile.z === this.z ? this.ring : e && e.range;
            if (!r) {
                continue;
            }
            if (inRange(r, tile.x, tile.y)) {
                tile.handle?.cancel?.();
                this._load(tile, e ? e.background !== false
                                   : !inRange(this.range, tile.x, tile.y));
            } else if (tile.loading) {
                this._drop(tile);
            }
        }
    }

    // True when no on-screen tile of the current level is still loading.
    // Ring tiles do not count: they are off screen.
    isSettled() {
        for (const tile of this.tiles.values()) {
            if (tile.z === this.z && tile.loading
                && inRange(this.range, tile.x, tile.y)) {
                return false;
            }
        }
        return true;
    }

    // True when every tile of level z in `range` has arrived.
    settledIn(z, range) {
        for (const { x, y } of rangeTiles(range)) {
            const tile = this.tiles.get(tileKey(z, x, y));
            if (!tile || tile.loading) {
                return false;
            }
        }
        return true;
    }

    // True when level z has at least one tile with something to draw.
    hasContentAt(z) {
        for (const tile of this.tiles.values()) {
            if (tile.z === z && tile.drawable) {
                return true;
            }
        }
        return false;
    }

    // Tiles of level z that have something to draw.
    drawableTilesAt(z) {
        const out = [];
        for (const tile of this.tiles.values()) {
            if (tile.z === z && tile.drawable) {
                out.push(tile);
            }
        }
        return out;
    }

    // Drop every tile whose level is not in `keepZs`.
    prune(keepZs) {
        const keep = new Set(keepZs);
        for (const tile of [...this.tiles.values()]) {
            if (!keep.has(tile.z)) {
                this._drop(tile);
            }
        }
    }

    clear() {
        for (const tile of [...this.tiles.values()]) {
            this._drop(tile);
        }
        this.z = null;
        this.range = null;
        this.ring = null;
        this.extra = [];
    }

    stats() {
        let loading = 0;
        let drawable = 0;
        for (const tile of this.tiles.values()) {
            if (tile.loading) {
                loading++;
            }
            if (tile.drawable) {
                drawable++;
            }
        }
        return { tiles: this.tiles.size, loading, drawable,
                 loads: this._loads, cancels: this._cancels };
    }

    _newTile(z, x, y) {
        const tile = { z, x, y, key: tileKey(z, x, y), drawable: null,
                       loading: false, background: false, handle: null,
                       generation: -1 };
        this.tiles.set(tile.key, tile);
        return tile;
    }

    _load(tile, background) {
        const generation = this._generation;
        const current = () => this.tiles.get(tile.key) === tile
                              && tile.generation === generation;
        tile.generation = generation;
        tile.loading = true;
        tile.background = background;
        this._loads++;
        const handle = this.source.load(
            { z: tile.z, x: tile.x, y: tile.y },
            {
                background,
                // Progressive paints while the tile is still loading.
                onPartial: (drawable) => {
                    if (!current()) {
                        this.source.release?.(drawable);
                        return;
                    }
                    this._setDrawable(tile, drawable);
                    this.onChange(tile);
                },
            });
        tile.handle = handle;
        handle.promise.then((drawable) => {
            if (!current()) {
                this.source.release?.(drawable);
                return;
            }
            if (drawable !== undefined) {
                this._setDrawable(tile, drawable);
            }
            tile.loading = false;
            tile.handle = null;
            this.onChange(tile);
        }, () => {
            // A failed tile counts as settled and keeps whatever it showed.
            if (!current()) {
                return;
            }
            tile.loading = false;
            tile.handle = null;
            this.onChange(tile);
        });
    }

    _setDrawable(tile, drawable) {
        if (tile.drawable && tile.drawable !== drawable) {
            this.source.release?.(tile.drawable);
        }
        tile.drawable = drawable || null;
    }

    _stale(tile) {
        return !tile.loading && tile.generation !== this._generation;
    }

    _inKeepBuffer(tile, r = this.range) {
        return inRange(growRange(r, this.keepBuffer), tile.x, tile.y);
    }

    _drop(tile) {
        if (tile.loading) {
            this._cancels++;
            tile.handle?.cancel?.();
        }
        if (tile.drawable) {
            this.source.release?.(tile.drawable);
        }
        tile.drawable = null;
        tile.loading = false;
        this.tiles.delete(tile.key);
    }
}
