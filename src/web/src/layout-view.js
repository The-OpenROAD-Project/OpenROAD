// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The 2D layout view, drawn with deck.gl.
//
// Owns the Deck, the view state and its animations, the tile slots (each a
// TileGrid over a tile source, all changing zoom level together through one
// ZoomSwap), the client-side shapes (selection outline, hover, rulers), DOM
// markers and image overlays.  The rest of the app talks to it through the
// methods returned by createLayoutView; nothing outside reads deck directly.
//
// World coordinates are view-math's: CSS px at zoom 0, y up, tile grid [0, T].

import {
    clampZoom, fitView, flyPath, growRange, growRect, project, rectsOverlap,
    snapTarget,
    tileRange, tileWorldBounds, tileZoomFor, unproject, viewWorldRect,
    zoomAround as zoomAroundTarget,
} from './view-math.js';
import { TileGrid, tileKey } from './tile-grid.js';
import { copyToCanvas2d } from './tile-sources.js';
import { rafCoalesce } from './ui-utils.js';
import { ZoomSwap } from './zoom-swap.js';

// Bottom to top.
export const DRAW_ORDER = Object.freeze({
    base: 10, overlay: 20, heatmap: 30, top: 40, pathImage: 50,
    selection: 60, highlight: 70, ruler: 80,
});

// Layers stack in list order, never by depth.
const noDepth = { depthCompare: 'always', depthWriteEnabled: false };

const ANIMATION_MS = 250;           // zoom and pan duration
const kMaxAnimatedZoomDelta = 4;           // larger jumps are not animated
const kInertiaMs = 300;
// The level above is kept loaded this many views around the view, so a zoom
// out around any point finds its tiles there.
const kPrefetchMargin = 1;
// Jumps further than this many views, or than kMaxAnimatedZoomDelta, fly.
const kFlightViews = 2;
const kFlightWaitMs = 300;                 // longest wait for the apex level
const kFlightMsPerS = 300;                 // per unit of flyPath length
const kFlightMinMs = 400;
const kFlightMaxMs = 1000;
const kInertiaWindowMs = 100;              // drag samples that set the speed
const kInertiaMinSpeed = 0.05;             // px/ms; slower releases just stop
const kFlashMs = 1200;                     // three 400 ms pulses
const kMaxTileZoom = 30;

// '#rgb', '#rrggbb' or [r, g, b(, a)] to [r, g, b, a] with alpha 0..255.
export function toRgba(color, opacity = 1) {
    if (Array.isArray(color)) {
        const a = color.length > 3 ? color[3] : 255;
        return [color[0], color[1], color[2], Math.round(a * opacity)];
    }
    let hex = String(color || '#000').replace('#', '');
    if (hex.length === 3) {
        hex = hex.split('').map((c) => c + c).join('');
    }
    const n = parseInt(hex.slice(0, 6), 16) || 0;
    return [(n >> 16) & 255, (n >> 8) & 255, n & 255, Math.round(255 * opacity)];
}

function easeOut(t) {
    // Close to cubic-bezier(0, 0, 0.25, 1).
    return 1 - Math.pow(1 - t, 3);
}

function easeInOut(t) {
    return t < 0.5 ? 4 * t * t * t : 1 - Math.pow(2 - 2 * t, 3) / 2;
}

function makeEmitter() {
    const handlers = new Map();
    return {
        on(types, fn) {
            for (const type of types.split(/\s+/)) {
                if (!handlers.has(type)) {
                    handlers.set(type, new Set());
                }
                handlers.get(type).add(fn);
            }
        },
        off(types, fn) {
            for (const type of types.split(/\s+/)) {
                handlers.get(type)?.delete(fn);
            }
        },
        emit(type, payload) {
            for (const fn of [...(handlers.get(type) || [])]) {
                fn(payload);
            }
        },
    };
}

// What draws the view: luma.gl reports a software renderer as gpuType 'cpu'.
function deviceInfo(d) {
    const info = (d && d.device && d.device.info) || {};
    return { gpuType: info.gpuType || 'unknown',
             renderer: info.renderer || '', vendor: info.vendor || '' };
}

// `opts`:
//   deckApi    the deck.gl namespace (window.deck); a missing one leaves a view
//              that keeps its state but draws nothing
//   prefs      { smooth(), doubleBuffer(), defer() }, read live
//   tileSize() CSS px of a tile; dpr() the device pixel ratio
//   isStatic   a saved report: no touch zoom
//   raf, now   injectable for tests
export function createLayoutView(container, opts = {}) {
    const deckApi = opts.deckApi !== undefined ? opts.deckApi : globalThis.deck;
    const copyCanvas = opts.copyCanvas || copyToCanvas2d;
    const prefs = {
        smooth: () => false, doubleBuffer: () => true, defer: () => false,
        ...(opts.prefs || {}),
    };
    const tileSize = opts.tileSize || (() => 240);
    const dpr = opts.dpr || (() => (typeof devicePixelRatio === 'number'
        ? devicePixelRatio : 1));
    const raf = opts.raf || ((fn) => requestAnimationFrame(fn));
    const now = opts.now || (() => performance.now());
    const events = makeEmitter();

    const vs = { target: [0, 0], zoom: 0, width: 1, height: 1 };
    const limits = { minZoom: -Infinity, maxZoom: kMaxTileZoom };
    let anim = null;          // { from, to, anchor, start, ms }
    let interacting = false;  // a drag or pinch from the controller
    let afterFrame = [];      // called once deck has drawn its next frame
    let rendererInfo = null;  // what draws, once deck's device is up
    let dragSamples = [];     // { t, target, zoom } while interacting
    let prefetchZ = null;     // level loaded in the background for a zoom out
    let flightTrail = null;   // a landed flight's pulled-out level, kept under
                              // the view until the destination level shows
    let prefetchPending = false;
    let moving = false;       // between movestart and moveend
    let flashUntil = 0;
    const cursorModes = new Set();   // 'ruler', 'label'
    let destroyed = false;

    // ─── DOM ────────────────────────────────────────────────────────────
    container.classList.add('or-view');
    const canvasWrap = document.createElement('div');
    canvasWrap.className = 'or-view-canvas';
    container.appendChild(canvasWrap);
    const markerLayer = document.createElement('div');
    markerLayer.className = 'or-view-markers';
    container.appendChild(markerLayer);

    // ─── Slots, shapes, markers ─────────────────────────────────────────
    const slots = new Map();      // id -> slot
    const overlays = new Map();   // id -> its layer specs
    const images = new Map();     // id -> its layer spec
    const markers = new Map();    // group -> { visible, items: Map id -> m }
    let textures = new WeakMap();  // drawable -> texture on the current device
    let doomedTextures = [];

    // One render per frame, however many changes ask for one.
    const scheduleRender = rafCoalesce(render, raf);

    const swap = new ZoomSwap({
        isEnabled: () => prefs.doubleBuffer(),
        isSmooth: () => prefs.smooth(),
        now,
        onChange: () => scheduleRender(),
    });

    // ─── Deck ───────────────────────────────────────────────────────────
    let deckgl = null;
    function controllerProps() {
        return {
            dragPan: true,
            dragRotate: false,
            scrollZoom: false,
            doubleClickZoom: false,
            touchZoom: !opts.isStatic,
            touchRotate: false,
            keyboard: false,
            // Inertia is startInertia's: deck's own only runs if its
            // transition props come back through the controlled viewState.
            inertia: false,
        };
    }
    function deckViewState() {
        return { target: [vs.target[0], vs.target[1], 0], zoom: vs.zoom,
                 minZoom: limits.minZoom, maxZoom: limits.maxZoom };
    }
    function createDeck() {
        return new deckApi.Deck({
            parent: canvasWrap,
            views: new deckApi.OrthographicView({
                id: 'main', flipY: false, controller: controllerProps(),
            }),
            viewState: deckViewState(),
            layers: [],
            useDevicePixels: true,
            // Save image copies the frame as it is drawn; keeping the buffer
            // readable outside it is for probes (?preservebuffer=1).
            deviceProps: { webgl: {
                preserveDrawingBuffer: !!opts.preserveDrawingBuffer } },
            getCursor: ({ isDragging }) => {
                if (cursorModes.size) {
                    return 'crosshair';
                }
                return isDragging ? 'grabbing' : 'default';
            },
            onViewStateChange: ({ viewState }) => {
                anim = null;
                setViewState(viewState.target, viewState.zoom);
                if (interacting) {
                    const t = now();
                    dragSamples.push({ t, target: [...vs.target],
                                       zoom: vs.zoom });
                    dragSamples = dragSamples.filter(
                        (d) => t - d.t <= kInertiaWindowMs);
                }
                return deckViewState();
            },
            onInteractionStateChange: (state) => {
                const busy = !!(state.isDragging || state.isPanning
                                || state.isZooming || state.inTransition);
                if (busy === interacting) {
                    return;
                }
                interacting = busy;
                if (busy) {
                    dragSamples = [];
                    startMove();
                    return;
                }
                // deck reports the end before the view state that goes with
                // it, and that update would cancel the glide.
                queueMicrotask(() => {
                    if (!interacting && !anim && !startInertia()) {
                        settleView();
                    }
                });
            },
            // The canvas only exists once the device is up.
            onLoad: () => {
                watchContext();
                rendererInfo = deviceInfo(deckgl);
                events.emit('renderer', rendererInfo);
                scheduleRender();
            },
            onAfterRender: () => {
                const waiting = afterFrame;
                afterFrame = [];
                waiting.forEach((fn) => fn());
            },
        });
    }

    // A lost WebGL context takes every texture with it.  A new Deck gets a
    // new canvas and context; the tiles' images are still here, so their
    // textures are rebuilt from them and nothing is re-requested.
    function watchContext() {
        const canvas = deckgl.getCanvas && deckgl.getCanvas();
        canvas?.addEventListener('webglcontextlost', (e) => {
            e.preventDefault();
            if (destroyed) {
                return;
            }
            events.emit('contextlost');
            const old = deckgl;
            deckgl = null;
            textures = new WeakMap();
            doomedTextures = [];
            try {
                old.finalize?.();
            } catch (err) {
                // The old device is already gone.
            }
            canvas.remove();
            try {
                deckgl = createDeck();
                events.emit('contextrestored');
            } catch (err) {
                events.emit('contextfailed', err);
            }
            scheduleRender();
        });
    }

    if (deckApi && typeof deckApi.Deck === 'function') {
        deckgl = createDeck();
    }

    // ─── Size ───────────────────────────────────────────────────────────
    // The canvas is sized to whole device pixels so tiles map 1:1 at rest.
    function measure() {
        const r = dpr();
        const w = Math.max(1, Math.floor(container.clientWidth * r) / r);
        const h = Math.max(1, Math.floor(container.clientHeight * r) / r);
        const changed = w !== vs.width || h !== vs.height;
        vs.width = w;
        vs.height = h;
        canvasWrap.style.width = w + 'px';
        canvasWrap.style.height = h + 'px';
        return changed;
    }
    measure();
    let resizeObserver = null;
    if (typeof ResizeObserver === 'function') {
        resizeObserver = new ResizeObserver(() => invalidateSize());
        resizeObserver.observe(container);
    }

    function invalidateSize() {
        if (measure()) {
            applyView();
            events.emit('resize', getSize());
        } else {
            scheduleRender();
        }
    }

    // ─── View state ─────────────────────────────────────────────────────
    function setViewState(target, zoom, { tiles = true } = {}) {
        const z = clampZoom(zoom, limits);
        const zoomChanged = z !== vs.zoom;
        vs.target = [target[0], target[1]];
        vs.zoom = z;
        applyView(zoomChanged, tiles);
    }

    function applyView(zoomChanged = false, tiles = true) {
        if (deckgl) {
            deckgl.setProps({ viewState: deckViewState() });
        }
        if (zoomChanged) {
            events.emit('zoom');
        }
        events.emit('move');
        if (tiles) {
            updateTiles();
        }
        scheduleRender();
    }

    function startMove() {
        if (!moving) {
            moving = true;
            events.emit('movestart');
        }
    }

    function settleView() {
        if (anim || interacting) {
            return;
        }
        // Touch zoom can leave a fractional level; rest on a whole one.
        const whole = Math.round(vs.zoom);
        if (Math.abs(whole - vs.zoom) > 1e-9) {
            animateTo({ target: zoomAroundTarget(vs, [vs.width / 2,
                                                      vs.height / 2], whole),
                        zoom: whole }, { anchor: null });
            return;
        }
        vs.target = snapTarget(vs, dpr());
        const ended = moving;
        moving = false;   // at rest before the tile update: z-1 may load
        applyView();
        if (ended) {
            events.emit('moveend');
            events.emit('zoomend');
        }
    }

    // A drag released while moving glides on, slowing to a stop.
    function startInertia() {
        const samples = dragSamples;
        dragSamples = [];
        if (!prefs.smooth() || samples.length < 2) {
            return false;
        }
        const first = samples[0];
        const last = samples[samples.length - 1];
        const dt = last.t - first.t;
        if (dt <= 0 || now() - last.t > kInertiaWindowMs
            || first.zoom !== last.zoom) {
            return false;
        }
        const vx = (last.target[0] - first.target[0]) / dt;
        const vy = (last.target[1] - first.target[1]) / dt;
        if (Math.hypot(vx, vy) * Math.pow(2, vs.zoom) < kInertiaMinSpeed) {
            return false;
        }
        // Same reach as deck's inertia: half the release speed for its length.
        animateTo({ target: [vs.target[0] + vx * kInertiaMs / 2,
                             vs.target[1] + vy * kInertiaMs / 2],
                    zoom: vs.zoom }, { ms: kInertiaMs });
        return true;
    }

    function animationMs(to) {
        if (!prefs.smooth()) {
            return 0;
        }
        if (Math.abs(to.zoom - vs.zoom) > kMaxAnimatedZoomDelta) {
            return 0;
        }
        return ANIMATION_MS;
    }

    // Move to `to` = { target, zoom }, animated when smoothing is on.  With
    // `anchor` (a screen point) the world point under it stays still.
    // `settle: false` leaves a jump unsettled, mid-gesture.  `curve` lets a
    // long jump fly (flyPath) instead of sliding or cutting.
    function animateTo(to, { anchor = null, ms = undefined, settle = true,
                             curve = false } = {}) {
        to = { target: to.target, zoom: clampZoom(to.zoom, limits) };
        const flight = curve && ms === undefined ? planFlight(to) : null;
        if (ms === undefined) {
            ms = flight ? flight.ms : animationMs(to);
        }
        startMove();
        if (to.zoom !== vs.zoom) {
            events.emit('zoomstart');
        }
        if (ms <= 0) {
            anim = null;
            // settleView updates the tiles once, at rest: an update while
            // moving would cancel the z-1 prefetch just to request it again.
            const atRest = settle && !interacting;
            setViewState(to.target, to.zoom, { tiles: !atRest });
            if (settle) {
                settleView();
            }
            return;
        }
        // A flight starts once the level it pulls out to has arrived.
        flightTrail = null;
        anim = { from: { ...vs, target: [...vs.target] }, to, anchor,
                 start: flight ? null : now(), created: now(), ms, flight };
        updateTiles();
        stepAnimation();
    }

    // The path, duration and pulled-out level of a jump far enough to fly,
    // or null.
    function planFlight(to) {
        if (!prefs.smooth()) {
            return null;
        }
        const dist = Math.hypot(to.target[0] - vs.target[0],
                                to.target[1] - vs.target[1]);
        const far = dist * Math.pow(2, Math.min(vs.zoom, to.zoom))
            > kFlightViews * Math.max(vs.width, vs.height);
        if (!far && Math.abs(to.zoom - vs.zoom) <= kMaxAnimatedZoomDelta) {
            return null;
        }
        const path = flyPath(vs, { ...vs, ...to });
        let zMin = Infinity;
        let rect = null;
        for (let i = 0; i <= 16; i++) {
            const p = path.at(i / 16);
            zMin = Math.min(zMin, p.zoom);
            const r = viewWorldRect({ ...vs, ...p });
            rect = rect ? [Math.min(rect[0], r[0]), Math.min(rect[1], r[1]),
                           Math.max(rect[2], r[2]), Math.max(rect[3], r[3])]
                : r;
        }
        const z = Math.max(0, tileZoomFor(zMin));
        const range = tileRange(rect, z, tileSize());
        return {
            path,
            ms: Math.min(kFlightMaxMs, Math.max(kFlightMinMs,
                                                kFlightMsPerS * path.S)),
            apex: range ? { z, range } : null,
        };
    }

    function apexReady(apex) {
        return !apex || [...slots.values()].every(
            (slot) => !slot.visible || slot.grid.settledIn(apex.z, apex.range));
    }

    function stepAnimation() {
        if (!anim || destroyed) {
            return;
        }
        if (anim.start === null) {
            if (!apexReady(anim.flight.apex)
                && now() - anim.created < kFlightWaitMs) {
                raf(stepAnimation);
                return;
            }
            anim.start = now();
        }
        const t = Math.min(1, (now() - anim.start) / anim.ms);
        const e = easeOut(t);
        let zoom = anim.from.zoom + (anim.to.zoom - anim.from.zoom) * e;
        let target;
        if (anim.flight) {
            ({ target, zoom } = anim.flight.path.at(easeInOut(t)));
        } else if (anim.anchor) {
            target = zoomAroundTarget(anim.from, anim.anchor, zoom);
        } else {
            target = [anim.from.target[0] + (anim.to.target[0]
                                             - anim.from.target[0]) * e,
                      anim.from.target[1] + (anim.to.target[1]
                                             - anim.from.target[1]) * e];
        }
        if (t >= 1) {
            const to = anim.to;
            flightTrail = anim.flight ? anim.flight.apex : null;
            anim = null;
            setViewState(to.target, to.zoom);
            settleView();
            return;
        }
        setViewState(target, zoom);
        raf(stepAnimation);
    }

    // ─── Tiles ──────────────────────────────────────────────────────────
    // The view the tiles are for: the destination of an animation.
    function tileView() {
        if (!anim) {
            return vs;
        }
        return { ...vs, target: anim.to.target, zoom: anim.to.zoom };
    }

    function updateTiles() {
        if (slots.size === 0) {
            return;
        }
        const hiding = prefs.doubleBuffer();
        const z = clampZoom(tileZoomFor(vs.zoom, {
            targetZoom: anim ? anim.to.zoom : null,
            hidingNewLevels: hiding,
        }), { minZoom: 0, maxZoom: kMaxTileZoom });
        const dest = tileView();
        const range = tileRange(viewWorldRect(dest), z, tileSize());
        const ring = growRange(range);
        const shown = swap.shownZ;
        const frozen = prefs.defer() && interacting;
        // Frozen, the grids keep their old level, so the swap must too.
        if (!frozen) {
            swap.setZoom(z, {
                hasContent: [...slots.values()].some(
                    (s) => s.visible && shown !== null
                           && s.grid.hasContentAt(shown)),
            });
        }
        // At rest only: it is background work and must not compete with the
        // view while it moves.
        const extra = [];
        if (z > 0 && !anim && !interacting && !moving
            && swap.pendingZ === null) {
            prefetchZ = z - 1;
            extra.push({ z: prefetchZ, range: tileRange(
                growRect(viewWorldRect(dest), kPrefetchMargin), prefetchZ,
                tileSize()) });
        }
        const apex = flightApex();
        if (apex) {
            extra.push({ ...apex, background: false });
        }
        // On screen now, which differs from `range` while animating.
        const hold = anim ? tileRange(viewWorldRect(vs), z, tileSize()) : null;
        for (const slot of slots.values()) {
            if (slot.visible && !frozen) {
                slot.grid.update({ z, range, ring, extra, hold });
            }
        }
        checkSwap();
    }

    // The level a flight pulls out to, while it flies and until it lands.
    function flightApex() {
        return (anim && anim.flight && anim.flight.apex) || flightTrail;
    }

    // Levels drawn under the shown ones where those have no tile: a flight's
    // pulled-out level, then the level waiting to be revealed.
    function underLevels(drawn) {
        const out = [];
        for (const z of [flightApex()?.z, swap.pendingZ]) {
            if (z != null && !out.includes(z)
                && !drawn.some((l) => l.z === z)) {
                out.push(z);
            }
        }
        return out;
    }

    // Once a level change has landed, fetch the level above it too.
    function schedulePrefetch() {
        if (prefetchPending) {
            return;
        }
        prefetchPending = true;
        queueMicrotask(() => {
            prefetchPending = false;
            if (!destroyed) {
                updateTiles();
            }
        });
    }

    function allSettled() {
        for (const slot of slots.values()) {
            if (slot.visible && !slot.grid.isSettled()) {
                return false;
            }
        }
        return true;
    }

    function checkSwap(tile) {
        const changed = swap.check({ settled: allSettled(), animating: !!anim });
        if (changed && swap.pendingZ === null) {
            schedulePrefetch();
        }
        let landed = false;
        if (flightTrail && !anim && swap.isIdle() && allSettled()) {
            flightTrail = null;
            landed = true;
        }
        // A tile of a level not drawn (the prefetch) changes nothing on screen.
        if (changed || landed || !tile || drawsLevel(tile.z)) {
            scheduleRender();
        }
    }

    function drawsLevel(z) {
        const drawn = swap.plan(now()).draw;
        return drawn.some((l) => l.z === z) || underLevels(drawn).includes(z);
    }

    function addTileSlot(id, { order, source, visible = true }) {
        const wrapped = {
            load: (coords, o) => source.load(coords, o),
            release: (drawable) => {
                const tex = drawable && textures.get(drawable);
                if (tex) {
                    textures.delete(drawable);
                    doomedTextures.push(tex);
                }
                source.release?.(drawable);
            },
        };
        const slot = {
            id, order, visible,
            grid: new TileGrid({ source: wrapped,
                                 onChange: (tile) => checkSwap(tile) }),
        };
        slots.set(id, slot);
        updateTiles();
        return {
            id,
            setVisible(v) {
                if (slot.visible === !!v) {
                    return;
                }
                slot.visible = !!v;
                if (!slot.visible) {
                    slot.grid.clear();
                }
                updateTiles();
            },
            isVisible: () => slot.visible,
            refresh() {
                if (slot.visible) {
                    slot.grid.refresh();
                    scheduleRender();
                }
            },
            remove() {
                slot.grid.clear();
                slots.delete(id);
                scheduleRender();
            },
        };
    }

    function textureFor(drawable) {
        if (!deckgl || !deckgl.device) {
            return drawable.image;
        }
        let tex = textures.get(drawable);
        if (!tex) {
            try {
                tex = deckgl.device.createTexture({
                    data: drawable.image,
                    width: drawable.width,
                    height: drawable.height,
                    mipLevels: 1,
                    sampler: { minFilter: 'linear', magFilter: 'linear',
                               addressModeU: 'clamp-to-edge',
                               addressModeV: 'clamp-to-edge' },
                });
            } catch (e) {
                return drawable.image;
            }
            textures.set(drawable, tex);
        }
        return tex;
    }

    // ─── Rendering ──────────────────────────────────────────────────────
    function flashFactor(t) {
        if (t >= flashUntil) {
            return 1;
        }
        const phase = ((kFlashMs - (flashUntil - t)) % 400) / 400;
        return 1 - 0.7 * (0.5 - 0.5 * Math.cos(2 * Math.PI * phase));
    }

    // World bounds per tile object: a tile keeps its z, x and y.
    let tileBounds = new WeakMap();
    let tileBoundsSize = null;
    function boundsOf(tile) {
        const T = tileSize();
        if (T !== tileBoundsSize) {
            tileBounds = new WeakMap();
            tileBoundsSize = T;
        }
        let bounds = tileBounds.get(tile);
        if (!bounds) {
            bounds = tileWorldBounds(tile.z, tile.x, tile.y, T);
            tileBounds.set(tile, bounds);
        }
        return bounds;
    }

    // A layer for each drawn tile of `slot` at level z that overlaps `view`;
    // `tag` keeps the ids of the tiles under the shown levels apart.
    function pushTileLayers(out, slot, z, view, tag, opacity) {
        for (const tile of slot.grid.drawableTilesAt(z)) {
            const bounds = boundsOf(tile);
            if (rectsOverlap(bounds, view)) {
                out.push({ order: slot.order, props: {
                    id: `${slot.id}:${tag}${tile.key}`,
                    image: textureFor(tile.drawable), bounds, opacity,
                    parameters: noDepth,
                } });
            }
        }
    }

    function tileLayers(t, view) {
        const out = [];
        const plan = swap.plan(t);
        if (!plan.fading && swap.pendingZ === null && plan.draw.length > 0) {
            const keep = [...plan.draw.map((l) => l.z), prefetchZ,
                          flightApex()?.z].filter((z) => z != null);
            for (const slot of slots.values()) {
                slot.grid.prune(keep);
            }
        }
        const flash = flashFactor(t);
        const ordered = [...slots.values()].sort((a, b) => a.order - b.order);
        const bottom = ordered.find((slot) => slot.visible);
        const under = underLevels(plan.draw);
        for (const slot of ordered) {
            if (!slot.visible) {
                continue;
            }
            if (slot === bottom && under.length > 0) {
                out.push(...underlay(slot, under, plan.draw, view, flash));
            }
            for (const level of plan.draw) {
                pushTileLayers(out, slot, level.z, view, '',
                               level.opacity * flash);
            }
        }
        return { layers: out, animate: plan.fading || t < flashUntil };
    }

    // While hidden levels load, their tiles fill what the shown levels do not
    // cover, such as the new edges of a zoom out.  A mask in the background
    // colour keeps them from showing through the shown tiles' clear pixels.
    function underlay(slot, zs, levels, view, flash) {
        const out = [];
        for (const z of zs) {
            pushTileLayers(out, slot, z, view, 'under:', flash);
        }
        if (out.length === 0) {
            return out;
        }
        // One CSS px of overlap, so no seam opens between two masks.
        const pad = 1 / Math.pow(2, vs.zoom);
        const masks = [];
        for (const tile of slot.grid.tiles.values()) {
            if (!levels.some((l) => l.z === tile.z)
                || (tile.loading && !tile.drawable)) {
                continue;
            }
            const bounds = boundsOf(tile);
            if (rectsOverlap(bounds, view)) {
                const [x0, y0, x1, y1] = bounds;
                masks.push([[x0 - pad, y0 - pad], [x1 + pad, y0 - pad],
                            [x1 + pad, y1 + pad], [x0 - pad, y1 + pad]]);
            }
        }
        if (masks.length) {
            out.push({ order: slot.order, kind: 'SolidPolygonLayer', props: {
                id: `${slot.id}:mask`, data: masks,
                getPolygon: (d) => d, getFillColor: backgroundRgba(),
                parameters: noDepth,
            } });
        }
        return out;
    }

    function backgroundRgba() {
        const m = (getComputedStyle(container).backgroundColor || '')
            .match(/[\d.]+/g);
        if (!m || m.length < 3 || (m.length > 3 && Number(m[3]) === 0)) {
            return [0, 0, 0, 255];
        }
        return [Number(m[0]), Number(m[1]), Number(m[2]), 255];
    }

    // The layer specs of one overlay, built when it is set so deck keeps the
    // same data until the shapes change.
    function overlaySpecs(id, shapes, order) {
        const out = [];
        const fills = [];
        const strokes = [];
        const circles = [];
        for (const s of shapes) {
            if (s.kind === 'circle') {
                circles.push(s);
                continue;
            }
            const pts = s.kind === 'rect'
                ? [[s.rect[0], s.rect[1]], [s.rect[2], s.rect[1]],
                   [s.rect[2], s.rect[3]], [s.rect[0], s.rect[3]]]
                : s.points;
            if (s.fill) {
                fills.push({ polygon: pts, color: s.fill });
            }
            if (s.stroke) {
                const closed = s.kind === 'rect' || s.closed;
                strokes.push({ path: closed ? [...pts, pts[0]] : pts,
                               color: s.stroke, width: s.width || 1,
                               dash: s.dash || null });
            }
        }
        if (fills.length) {
            out.push({ order, kind: 'SolidPolygonLayer', props: {
                id: `${id}:fill`, data: fills,
                getPolygon: (d) => d.polygon, getFillColor: (d) => d.color,
                parameters: noDepth,
            } });
        }
        const solid = strokes.filter((d) => !d.dash);
        const dashed = strokes.filter((d) => d.dash);
        if (solid.length) {
            out.push({ order, kind: 'PathLayer', props: {
                id: `${id}:stroke`, data: solid, widthUnits: 'pixels',
                getPath: (d) => d.path, getColor: (d) => d.color,
                getWidth: (d) => d.width,
                parameters: noDepth,
            } });
        }
        if (dashed.length) {
            const props = {
                id: `${id}:dash`, data: dashed, widthUnits: 'pixels',
                getPath: (d) => d.path, getColor: (d) => d.color,
                getWidth: (d) => d.width,
                // The extension counts dashes in line widths.
                getDashArray: (d) => d.dash.map((v) => v / d.width),
                dashJustified: true,
                parameters: noDepth,
            };
            if (deckApi?.PathStyleExtension) {
                props.extensions = [
                    new deckApi.PathStyleExtension({ dash: true })];
            }
            out.push({ order, kind: 'PathLayer', props });
        }
        if (circles.length) {
            out.push({ order, kind: 'ScatterplotLayer', props: {
                id: `${id}:circle`, data: circles,
                radiusUnits: 'pixels', lineWidthUnits: 'pixels',
                getPosition: (d) => d.center, getRadius: (d) => d.radius,
                stroked: true, filled: true,
                getLineColor: (d) => d.stroke || [0, 0, 0, 0],
                getFillColor: (d) => d.fill || [0, 0, 0, 0],
                getLineWidth: (d) => d.width || 1,
                parameters: noDepth,
            } });
        }
        return out;
    }

    function shapeLayers() {
        const out = [];
        for (const specs of overlays.values()) {
            out.push(...specs);
        }
        out.push(...images.values());
        return out;
    }

    function buildDeckLayers(specs) {
        return specs.map(
            (spec) => new deckApi[spec.kind || 'BitmapLayer'](spec.props));
    }

    let lastLayerCount = 0;
    function render() {
        if (destroyed) {
            return;
        }
        const t = now();
        // deck may draw a new view state before this runs again, so tiles
        // half a view off screen are kept: a pan or a one-level zoom out
        // then finds them already there.
        const tiles = tileLayers(t, growRect(viewWorldRect(vs), 0.5));
        const specs = [...tiles.layers, ...shapeLayers()]
            .sort((a, b) => a.order - b.order);
        lastLayerCount = specs.length;
        if (deckgl) {
            deckgl.setProps({ layers: buildDeckLayers(specs) });
        }
        for (const tex of doomedTextures.splice(0)) {
            tex.destroy?.();
        }
        positionMarkers();
        if (tiles.animate) {
            scheduleRender();
        }
        events.emit('render');
    }

    // Share of the view inside the tile grid that shows a settled base tile
    // of a level being drawn: the browser probes' blank metric.
    function coverage(nx = 32, ny = 20) {
        const slot = slots.get('base');
        if (!slot) {
            return null;
        }
        const T = tileSize();
        const drawn = swap.plan(now()).draw;
        const levels = [...drawn, ...underLevels(drawn).map((z) => ({ z }))];
        let inGrid = 0;
        let shown = 0;
        for (let j = 0; j < ny; j++) {
            for (let i = 0; i < nx; i++) {
                const [wx, wy] = unproject([(i + 0.5) * vs.width / nx,
                                            (j + 0.5) * vs.height / ny], vs);
                if (wx < 0 || wy < 0 || wx >= T || wy >= T) {
                    continue;
                }
                inGrid++;
                const hit = levels.some(({ z }) => {
                    const S = T / 2 ** z;
                    const tile = slot.grid.tiles.get(tileKey(
                        z, Math.floor(wx / S), Math.floor((T - wy) / S)));
                    return tile && (tile.drawable || !tile.loading);
                });
                if (hit) {
                    shown++;
                }
            }
        }
        return inGrid ? shown / inGrid : null;
    }

    // ─── Markers ────────────────────────────────────────────────────────
    function markerGroup(group) {
        if (!markers.has(group)) {
            markers.set(group, { visible: true, items: new Map() });
        }
        return markers.get(group);
    }

    function placeMarker(m, visible) {
        const [x, y] = project(m.world, vs);
        m.el.style.display = visible ? '' : 'none';
        m.el.style.transform = `translate(${x}px, ${y}px)`
            + (m.center ? ' translate(-50%, -50%)' : '');
    }

    function placeGroup(g) {
        for (const m of g.items.values()) {
            placeMarker(m, g.visible);
        }
    }

    function positionMarkers() {
        for (const g of markers.values()) {
            placeGroup(g);
        }
    }

    // ─── Public API ─────────────────────────────────────────────────────
    function getSize() {
        return { width: vs.width, height: vs.height };
    }

    const view = {
        DRAW_ORDER,
        getContainer: () => container,
        getRendererInfo: () => rendererInfo,
        getSize,
        getZoom: () => vs.zoom,
        // The zoom an animation in flight will end on.
        getTargetZoom: () => (anim ? anim.to.zoom : vs.zoom),
        getCenter: () => [...vs.target],
        getViewState: () => ({ ...vs, target: [...vs.target] }),
        isAnimating: () => !!anim,
        isMoving: () => moving,
        project: (w) => project(w, vs),
        unproject: (p) => unproject(p, vs),
        on: events.on,
        off: events.off,

        setLimits({ minZoom, maxZoom } = {}) {
            if (minZoom !== undefined) {
                limits.minZoom = minZoom;
            }
            if (maxZoom !== undefined) {
                limits.maxZoom = maxZoom;
            }
            const z = clampZoom(vs.zoom, limits);
            if (z !== vs.zoom) {
                setViewState(vs.target, z);
            } else if (deckgl) {
                deckgl.setProps({ viewState: deckViewState() });
            }
            events.emit('limits');
        },
        getLimits: () => ({ ...limits }),

        setView(center, zoom = vs.zoom, { animate } = {}) {
            const to = { target: [center[0], center[1]], zoom };
            animateTo(to, animate === false ? { ms: 0 } : { curve: true });
        },

        fitWorldRect(rect, { padding = [0, 0], animate } = {}) {
            const fit = fitView(rect, vs, { padding, ...limits });
            animateTo(fit, animate === false ? { ms: 0 } : { curve: true });
        },

        // Zoom by `delta` levels from the level an animation is heading to,
        // keeping the point under `screenPt` still (the centre when null).
        // `settle: false` (with `animate: false`) keeps a fractional zoom for
        // a gesture still under way; a later call settles it.
        zoomAround(screenPt, delta, { animate, settle = true } = {}) {
            const zoom = clampZoom(view.getTargetZoom() + delta, limits);
            const base = tileView();
            const pt = screenPt || [vs.width / 2, vs.height / 2];
            const target = zoomAroundTarget(base, pt, zoom);
            animateTo({ target, zoom },
                      animate === false ? { ms: 0, settle } : { anchor: screenPt
                          ? pt : null });
        },
        zoomIn: () => view.zoomAround(null, 1),
        zoomOut: () => view.zoomAround(null, -1),

        panByPx(dx, dy, { animate } = {}) {
            const base = tileView();
            const s = Math.pow(2, base.zoom);
            const target = [base.target[0] + dx / s, base.target[1] - dy / s];
            const ms = (animate === false || !prefs.smooth()) ? 0 : ANIMATION_MS;
            animateTo({ target, zoom: base.zoom }, { ms });
        },

        invalidateSize,
        // The preferences are read live; re-plan the tiles for defer and
        // double buffering.
        applyPrefs() {
            updateTiles();
        },

        addTileSlot,

        setOverlay(id, shapes, { order = DRAW_ORDER.selection } = {}) {
            if (!shapes || shapes.length === 0) {
                overlays.delete(id);
            } else {
                overlays.set(id, overlaySpecs(id, shapes, order));
            }
            scheduleRender();
        },
        hasOverlay: (id) => overlays.has(id),

        setImageOverlay(id, spec, { order = DRAW_ORDER.pathImage } = {}) {
            if (!spec) {
                images.delete(id);
            } else {
                images.set(id, { order, kind: 'BitmapLayer', props: {
                    id: `image:${id}`, image: spec.image, bounds: spec.rect,
                    parameters: noDepth,
                } });
            }
            scheduleRender();
        },

        // A DOM element pinned to a world point: its top-left corner, or its
        // centre with `center`.
        addMarker(group, id, world, el, { center = false } = {}) {
            const g = markerGroup(group);
            const old = g.items.get(id);
            if (old) {
                old.el.remove();
            }
            el.classList.add('or-view-marker');
            markerLayer.appendChild(el);
            const m = { el, world: [world[0], world[1]], center };
            g.items.set(id, m);
            placeMarker(m, g.visible);
            return el;
        },
        moveMarker(group, id, world) {
            const g = markerGroup(group);
            const m = g.items.get(id);
            if (m) {
                m.world = [world[0], world[1]];
                placeMarker(m, g.visible);
            }
        },
        clearMarkers(group) {
            const g = markerGroup(group);
            for (const m of g.items.values()) {
                m.el.remove();
            }
            g.items.clear();
        },
        setMarkerGroupVisible(group, visible) {
            const g = markerGroup(group);
            g.visible = !!visible;
            placeGroup(g);
        },

        // The ruler and label tools each hold a crosshair mode.
        setCursorMode(mode, on = true) {
            if (on) {
                cursorModes.add(mode);
            } else {
                cursorModes.delete(mode);
            }
            container.classList.toggle(`${mode}-mode-cursor`, !!on);
            // deck sets the cursor on its canvas from getCursor, but only on
            // the next pointer event; apply the change now.
            const canvas = deckgl && deckgl.getCanvas && deckgl.getCanvas();
            if (canvas) {
                canvas.style.cursor = cursorModes.size ? 'crosshair' : 'default';
            }
        },

        // The DRC widget's attention pulse on the tiles.
        flashTiles() {
            flashUntil = now() + kFlashMs;
            scheduleRender();
        },

        // Resolves once every visible slot has loaded what is on screen and
        // no level change or animation is in progress.
        whenTilesSettled({ timeoutMs = 6000 } = {}) {
            const deadline = now() + timeoutMs;
            return new Promise((resolve) => {
                const poll = () => {
                    if ((allSettled() && swap.isIdle() && !anim && !moving)
                        || now() >= deadline) {
                        resolve();
                        return;
                    }
                    setTimeout(poll, 32);
                };
                poll();
            });
        },

        // `{ image, rect }`: a copy of the frame with the current layers and
        // where the canvas sits, for Save image.  deck applies new layers in
        // its own frame, so wait for one and copy it while it is drawn.
        captureCanvas({ timeoutMs = 500 } = {}) {
            if (!deckgl?.getCanvas()) {
                return Promise.resolve(null);
            }
            render();
            return new Promise((resolve) => {
                let timer = null;
                const done = () => {
                    clearTimeout(timer);
                    afterFrame = afterFrame.filter((fn) => fn !== done);
                    const canvas = deckgl?.getCanvas();
                    resolve(canvas ? { image: copyCanvas(canvas),
                                       rect: canvas.getBoundingClientRect() }
                        : null);
                };
                afterFrame.push(done);
                // A hidden page gets no frames.
                timer = setTimeout(() => {
                    deckgl?.redraw('capture');
                    done();
                }, timeoutMs);
            });
        },

        redraw: () => scheduleRender(),

        debugSnapshot() {
            const plan = swap.plan();
            return {
                zoom: vs.zoom, target: [...vs.target],
                width: vs.width, height: vs.height,
                animating: !!anim, moving, interacting,
                shownZ: swap.shownZ, pendingZ: swap.pendingZ,
                fading: plan.fading, drawLevels: plan.draw,
                layers: lastLayerCount, coverage: coverage(),
                overlays: [...overlays.keys()],
                markers: Object.fromEntries([...markers].map(
                    ([group, g]) => [group, g.items.size])),
                slots: [...slots.values()].map((s) => ({
                    id: s.id, visible: s.visible, settled: s.grid.isSettled(),
                    ...s.grid.stats(),
                })),
            };
        },

        destroy() {
            destroyed = true;
            resizeObserver?.disconnect();
            swap.dispose();
            for (const slot of slots.values()) {
                slot.grid.clear();
            }
            deckgl?.finalize?.();
        },
    };
    return view;
}
