// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Mouse, wheel and keyboard on the layout view, as DOM listeners over the view
// API.  Drag-panning is left to the renderer; everything that zooms, selects
// or draws a box is here.

const kClickTolerancePx = 3;   // a press that moves less is a click
const kBandMinPx = 4;          // a smaller right-drag is a click
const kNotchMinPx = 50;        // a mouse wheel step in pixels is at least this
const kPxPerLevel = 100;       // trackpad scroll that zooms one level
const kGestureIdleMs = 200;    // a pause this long ends a trackpad gesture
const kGestureMinLevels = 0.2; // a smaller gesture springs back
const kMacWheelStep = 4.000244140625;  // Chrome on macOS: one mouse wheel step

// Steps from a mouse wheel: lines or pages, whole pixel steps of at least
// kNotchMinPx, or multiples of kMacWheelStep, with no sideways part.  A
// trackpad scroll or pinch is a stream of small deltas instead.
export function isWheelNotch(e) {
    if (e.deltaMode !== 0) {
        return true;
    }
    if (e.deltaX !== 0 || e.deltaY === 0) {
        return false;
    }
    return (Number.isInteger(e.deltaY) && Math.abs(e.deltaY) >= kNotchMinPx)
        || e.deltaY % kMacWheelStep === 0;
}

// The whole levels a trackpad gesture of `d` levels ends on: at least one
// past kGestureMinLevels, the nearest after that.
export function gestureSteps(d) {
    const a = Math.abs(d);
    return a < kGestureMinLevels ? 0 : Math.sign(d) * Math.max(1, Math.round(a));
}

// A wheel event's scroll amount in CSS pixels.  deltaMode is not always pixels:
// Firefox reports lines, and a page-mode event (some remote-desktop stacks)
// means a viewport at a time.
export function wheelDeltaPx(e, container) {
    const kLineHeightPx = 16;
    let scale = 1;
    if (e.deltaMode === 1) {
        scale = kLineHeightPx;
    } else if (e.deltaMode === 2) {
        scale = container.clientHeight || kLineHeightPx;
    }
    return [e.deltaX * scale, e.deltaY * scale];
}

// Arrow key -> pan in screen px; Shift pans 3x.
export function arrowPanOffset(key, step, shift) {
    const s = shift ? 3 * step : step;
    switch (key) {
    case 'ArrowLeft': return [-s, 0];
    case 'ArrowRight': return [s, 0];
    case 'ArrowUp': return [0, -s];
    case 'ArrowDown': return [0, s];
    default: return null;
    }
}

// `opts`:
//   isStatic()      a saved report: only drag and the arrow keys stay on
//   wheelZoom()     Options > "Mouse wheel mapped to zoom"
//   arrowStep()     Options > "Arrow keys scroll step", CSS px
//   onClick(world, event), onContextClick(world, event)
//   onMove(world, event), onLeave(event)
//   contextEnabled() false while a ruler is being placed
export function installViewInput(view, opts = {}) {
    const container = view.getContainer();
    const isStatic = opts.isStatic || (() => false);
    const wheelZoom = opts.wheelZoom || (() => true);
    const arrowStep = opts.arrowStep || (() => 80);
    const gestureIdleMs = opts.gestureIdleMs ?? kGestureIdleMs;
    const contextEnabled = opts.contextEnabled || (() => true);
    const listeners = [];
    const on = (target, type, fn, options) => {
        target.addEventListener(type, fn, options);
        listeners.push(() => target.removeEventListener(type, fn, options));
    };
    const local = (e) => {
        const r = container.getBoundingClientRect();
        return [e.clientX - r.left, e.clientY - r.top];
    };
    const worldAt = (e) => view.unproject(local(e));
    // On the layout itself: not on a control, HUD or marker over it, which
    // handle their own input.
    const onView = (e) => e.target === container
        || !!e.target?.closest?.('.or-view-canvas');

    if (container.tabIndex < 0) {
        container.tabIndex = 0;
    }

    // A trackpad zoom follows the fingers at fractional levels, then rests on
    // a whole level once they stop.
    let gesture = null;   // { z0, pt, timer }
    function zoomGesture(pt, levels) {
        if (!gesture) {
            gesture = { z0: view.getZoom() };
        }
        gesture.pt = pt;
        clearTimeout(gesture.timer);
        gesture.timer = setTimeout(endGesture, gestureIdleMs);
        view.zoomAround(pt, levels, { animate: false, settle: false });
    }
    function endGesture() {
        if (!gesture) {
            return;
        }
        const g = gesture;
        gesture = null;
        clearTimeout(g.timer);
        const z = view.getZoom();
        view.zoomAround(g.pt, g.z0 + gestureSteps(z - g.z0) - z);
    }
    listeners.push(() => {
        clearTimeout(gesture?.timer);
        gesture = null;
    });

    // Wheel: the preference and Ctrl pick between panning and zooming around
    // the cursor (Qt's LayoutScroll::wheelEvent), one level per mouse step.
    // A pinch always zooms.
    on(container, 'wheel', (e) => {
        e.preventDefault();
        if (isStatic()) {
            return;
        }
        const notch = isWheelNotch(e);
        if (!notch && e.ctrlKey) {
            // Chrome and Firefox send a pinch as ctrl+wheel with
            // deltaY = -100 ln(scale).
            zoomGesture(local(e), -e.deltaY / (100 * Math.LN2));
            return;
        }
        if (wheelZoom() === e.ctrlKey) {
            const [dx, dy] = wheelDeltaPx(e, container);
            view.panByPx(dx, dy, { animate: false });
            return;
        }
        if (!notch) {
            zoomGesture(local(e), -e.deltaY / kPxPerLevel);
            return;
        }
        endGesture();
        view.zoomAround(local(e), e.deltaY < 0 ? 1 : -1);
    }, { passive: false });

    on(container, 'keydown', (e) => {
        if (e.altKey || e.ctrlKey || e.metaKey) {
            return;
        }
        const offset = arrowPanOffset(e.key, arrowStep(), e.shiftKey);
        if (offset) {
            e.preventDefault();
            view.panByPx(offset[0], offset[1]);
            return;
        }
        if (isStatic()) {
            return;
        }
        const zoomKeys = { '+': 1, '=': 1, '-': -1, '_': -1 };
        if (e.key in zoomKeys) {
            e.preventDefault();
            view.zoomAround(null, zoomKeys[e.key] * (e.shiftKey ? 3 : 1));
        }
    });

    // Left button: a press that moves no more than the tolerance is a click,
    // however long it is held.  Shift+drag draws a zoom box instead of panning.
    let press = null;
    let box = null;
    on(container, 'pointerdown', (e) => {
        if (e.button !== 0 || !onView(e)) {
            return;
        }
        container.focus({ preventScroll: true });
        press = { x: e.clientX, y: e.clientY, moved: false,
                  box: e.shiftKey && !isStatic() };
        if (press.box) {
            e.stopPropagation();   // keep the renderer from panning
        }
    }, { capture: true });

    on(window, 'pointermove', (e) => {
        if (!press) {
            return;
        }
        const dx = e.clientX - press.x;
        const dy = e.clientY - press.y;
        if (Math.abs(dx) > kClickTolerancePx
            || Math.abs(dy) > kClickTolerancePx) {
            press.moved = true;
        }
        if (press.box && press.moved) {
            box = drawBand(box, press, e);
        }
    });

    on(window, 'pointerup', (e) => {
        if (!press || e.button !== 0) {
            return;
        }
        const p = press;
        press = null;
        if (box) {
            box.remove();
            box = null;
        }
        if (p.box && p.moved) {
            fitScreenBox(p, e);
            return;
        }
        if (!p.moved && opts.onClick && isInside(e)) {
            opts.onClick(worldAt(e), e);
        }
    });

    on(container, 'dblclick', (e) => {
        if (isStatic() || !onView(e)) {
            return;
        }
        view.zoomAround(local(e), e.shiftKey ? -1 : 1);
    });

    // Right button: drag for a rubber-band zoom, click for the context menu.
    let band = null;
    let bandEl = null;
    on(container, 'contextmenu', (e) => e.preventDefault());
    on(container, 'pointerdown', (e) => {
        if (e.button !== 2 || isStatic() || !onView(e)) {
            return;
        }
        e.stopPropagation();
        band = { x: e.clientX, y: e.clientY };
    }, { capture: true });
    on(window, 'pointermove', (e) => {
        if (!band) {
            return;
        }
        const dx = e.clientX - band.x;
        const dy = e.clientY - band.y;
        if (bandEl || (Math.abs(dx) >= kBandMinPx && Math.abs(dy) >= kBandMinPx)) {
            bandEl = drawBand(bandEl, band, e);
        }
    });
    on(window, 'pointerup', (e) => {
        if (!band || e.button !== 2) {
            return;
        }
        const start = band;
        band = null;
        if (bandEl) {
            bandEl.remove();
            bandEl = null;
            fitScreenBox(start, e);
            return;
        }
        if (isInside(e) && contextEnabled() && opts.onContextClick) {
            opts.onContextClick(worldAt(e), e);
        }
    });

    on(container, 'pointermove', (e) => {
        if (opts.onMove) {
            opts.onMove(worldAt(e), e);
        }
    });
    on(container, 'pointerleave', (e) => {
        if (opts.onLeave) {
            opts.onLeave(e);
        }
    });

    function isInside(e) {
        const r = container.getBoundingClientRect();
        return e.clientX >= r.left && e.clientX <= r.right
            && e.clientY >= r.top && e.clientY <= r.bottom;
    }

    function fitScreenBox(start, e) {
        const r = container.getBoundingClientRect();
        const a = view.unproject([start.x - r.left, start.y - r.top]);
        const b = view.unproject([e.clientX - r.left, e.clientY - r.top]);
        view.fitWorldRect([Math.min(a[0], b[0]), Math.min(a[1], b[1]),
                           Math.max(a[0], b[0]), Math.max(a[1], b[1])]);
    }

    return {
        dispose() {
            for (const off of listeners.splice(0)) {
                off();
            }
        },
    };
}

// The rubber-band rectangle, in page coordinates.
function drawBand(el, start, e) {
    if (!el) {
        el = document.createElement('div');
        el.className = 'rubber-band';
        document.body.appendChild(el);
    }
    el.style.left = Math.min(start.x, e.clientX) + 'px';
    el.style.top = Math.min(start.y, e.clientY) + 'px';
    el.style.width = Math.abs(e.clientX - start.x) + 'px';
    el.style.height = Math.abs(e.clientY - start.y) + 'px';
    return el;
}
