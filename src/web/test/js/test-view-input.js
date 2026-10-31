// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it } from 'node:test';
import assert from 'node:assert/strict';

const { installViewInput, wheelDeltaPx, arrowPanOffset, isWheelNotch,
        gestureSteps } = await import('../../src/view-input.js');

// A view that records calls; world = screen / 2 so results are checkable.
function makeView() {
    const container = document.createElement('div');
    document.body.appendChild(container);
    // jsdom lays nothing out, so give the view a size.
    container.getBoundingClientRect = () => ({ left: 0, top: 0, right: 1000,
                                               bottom: 800, width: 1000,
                                               height: 800 });
    const calls = [];
    // [on, calls made before it], kept apart so `calls` reads as before.
    const gestures = [];
    const view = {
        getContainer: () => container,
        unproject: ([x, y]) => [x / 2, y / 2],
        zoom: 3,
        getZoom: () => view.zoom,
        panByPx: (dx, dy, opts) => calls.push(['pan', dx, dy, opts]),
        zoomAround: (pt, delta, opts) => {
            calls.push(opts ? ['zoom', pt, delta, opts] : ['zoom', pt, delta]);
            view.zoom += delta;
        },
        fitWorldRect: (rect) => calls.push(['fit', rect]),
        setGesture: (on) => gestures.push([on, calls.length]),
    };
    return { view, container, calls, gestures };
}

function fire(target, type, init = {}) {
    const Ctor = type === 'wheel' ? window.WheelEvent
        : type.startsWith('key') ? window.KeyboardEvent : window.MouseEvent;
    const event = new Ctor(type, { bubbles: true, cancelable: true, ...init });
    target.dispatchEvent(event);
    return event;
}

describe('wheel', () => {
    it('zooms one level around the cursor when the preference says zoom', () => {
        const { view, container, calls } = makeView();
        installViewInput(view, { wheelZoom: () => true });
        const e = fire(container, 'wheel', { deltaY: -100, clientX: 40, clientY: 20 });
        assert.ok(e.defaultPrevented);
        assert.deepEqual(calls, [['zoom', [40, 20], 1]]);
    });

    it('pans with Ctrl held, and the other way round with the preference off', () => {
        const { view, container, calls } = makeView();
        let zoomPref = true;
        installViewInput(view, { wheelZoom: () => zoomPref });
        fire(container, 'wheel', { deltaY: 100, ctrlKey: true });
        zoomPref = false;
        fire(container, 'wheel', { deltaY: 100 });
        fire(container, 'wheel', { deltaY: 100, ctrlKey: true });
        assert.deepEqual(calls.map((c) => c[0]), ['pan', 'pan', 'zoom']);
        assert.equal(calls[2][2], -1);
    });

    it('pans horizontally from deltaX', () => {
        const { view, container, calls } = makeView();
        installViewInput(view, { wheelZoom: () => false });
        fire(container, 'wheel', { deltaX: 25, deltaY: 0 });
        assert.deepEqual(calls, [['pan', 25, 0, { animate: false }]]);
    });

    it('does nothing in a saved report', () => {
        const { view, container, calls } = makeView();
        installViewInput(view, { isStatic: () => true });
        fire(container, 'wheel', { deltaY: -100 });
        assert.equal(calls.length, 0);
    });

    it('converts line and page deltas to pixels', () => {
        const container = { clientHeight: 500 };
        assert.deepEqual(wheelDeltaPx({ deltaX: 0, deltaY: 3, deltaMode: 1 },
                                      container), [0, 48]);
        assert.deepEqual(wheelDeltaPx({ deltaX: 0, deltaY: 1, deltaMode: 2 },
                                      container), [0, 500]);
    });
});

// A trackpad sends a stream of small deltas, and a pinch arrives as
// ctrl+wheel; a mouse wheel sends whole steps.
describe('trackpad', () => {
    const wait = (ms) => new Promise((resolve) => setTimeout(resolve, ms));
    const LEVEL = 100 * Math.LN2;   // pinch deltaY of one level

    it('tells mouse steps from trackpad streams', () => {
        const ev = (deltaY, more = {}) => ({ deltaMode: 0, deltaX: 0, deltaY,
                                              ...more });
        assert.ok(isWheelNotch(ev(100)));
        assert.ok(isWheelNotch(ev(-120)));
        assert.ok(isWheelNotch(ev(3, { deltaMode: 1 })));
        assert.ok(!isWheelNotch(ev(12)));
        assert.ok(!isWheelNotch(ev(4)));
        assert.ok(!isWheelNotch(ev(3.5)));
        assert.ok(!isWheelNotch(ev(100, { deltaX: 2 })));
        assert.ok(!isWheelNotch(ev(90.9)));
    });

    // Chrome on macOS sends a mouse wheel step as 4.000244140625 px.
    it('zooms a whole level on a macOS mouse wheel step', () => {
        const { view, container, calls } = makeView();
        installViewInput(view);
        assert.ok(isWheelNotch({ deltaMode: 0, deltaX: 0,
                                 deltaY: 4.000244140625 }));
        assert.ok(isWheelNotch({ deltaMode: 0, deltaX: 0,
                                 deltaY: -12.000732421875 }));
        fire(container, 'wheel', { deltaY: -4.000244140625, clientX: 10,
                                   clientY: 20 });
        assert.deepEqual(calls, [['zoom', [10, 20], 1]]);
    });

    it('ends a gesture on at least one level past 0.2, the nearest after', () => {
        assert.equal(gestureSteps(0.15), 0);
        assert.equal(gestureSteps(0.2), 1);
        assert.equal(gestureSteps(0.6), 1);
        assert.equal(gestureSteps(1.2), 1);
        assert.equal(gestureSteps(1.6), 2);
        assert.equal(gestureSteps(-0.25), -1);
        assert.equal(gestureSteps(-2.4), -2);
        assert.equal(Math.abs(gestureSteps(-0.1)), 0);
    });

    it('zooms on a pinch even when the preference makes ctrl+wheel pan',
       async () => {
        const { view, container, calls } = makeView();
        installViewInput(view, { wheelZoom: () => true, gestureIdleMs: 20 });
        fire(container, 'wheel', { deltaY: -LEVEL / 2, ctrlKey: true,
                                   clientX: 40, clientY: 20 });
        fire(container, 'wheel', { deltaY: -LEVEL / 2, ctrlKey: true,
                                   clientX: 40, clientY: 20 });
        assert.deepEqual(calls.map((c) => c[0]), ['zoom', 'zoom']);
        assert.ok(Math.abs(view.zoom - 4) < 1e-9, 'one level, as the fingers');
        assert.deepEqual(calls[0][3], { animate: false, settle: false });
        await wait(40);
        // The pause settles it, around the fingers.
        assert.deepEqual(calls[2].slice(0, 2), ['zoom', [40, 20]]);
        assert.equal(calls[2].length, 3, 'animated and settled');
        assert.ok(Math.abs(view.zoom - 4) < 1e-9);
    });

    it('zooms in proportion to a trackpad scroll and rests on a whole level',
       async () => {
        const { view, container, calls } = makeView();
        installViewInput(view, { wheelZoom: () => true, gestureIdleMs: 20 });
        for (let i = 0; i < 3; i++) {
            fire(container, 'wheel', { deltaY: -12.5 });
        }
        assert.ok(Math.abs(view.zoom - 3.375) < 1e-9);
        await wait(40);
        assert.equal(view.zoom, 4);
        assert.equal(calls.length, 4);
    });

    it('springs back from a nudge under 0.2 of a level', async () => {
        const { view, container } = makeView();
        installViewInput(view, { wheelZoom: () => true, gestureIdleMs: 20 });
        fire(container, 'wheel', { deltaY: 10 });
        await wait(40);
        assert.equal(view.zoom, 3);
    });

    it('pans on a trackpad scroll with the preference off', () => {
        const { view, container, calls } = makeView();
        installViewInput(view, { wheelZoom: () => false });
        fire(container, 'wheel', { deltaY: 12, deltaX: 5 });
        assert.deepEqual(calls, [['pan', 5, 12, { animate: false }]]);
    });

    it('holds the view through a scroll pan until it pauses', async () => {
        const { view, container, gestures } = makeView();
        installViewInput(view, { wheelZoom: () => false, gestureIdleMs: 20 });
        fire(container, 'wheel', { deltaY: 12 });
        fire(container, 'wheel', { deltaY: 12 });
        assert.deepEqual(gestures, [[true, 0]]);
        await wait(40);
        assert.deepEqual(gestures, [[true, 0], [false, 2]]);
    });

    // Let go before the settling zoom, the view would load the fractional
    // level the fingers left it on.
    it('lets go of a pinch only after settling it', async () => {
        const { view, container, gestures } = makeView();
        installViewInput(view, { wheelZoom: () => true, gestureIdleMs: 20 });
        fire(container, 'wheel', { deltaY: -LEVEL / 2, ctrlKey: true });
        fire(container, 'wheel', { deltaY: -LEVEL / 2, ctrlKey: true });
        assert.deepEqual(gestures, [[true, 0]]);
        await wait(40);
        assert.deepEqual(gestures, [[true, 0], [false, 3]]);
    });

    it('settles a gesture before a mouse step', () => {
        const { view, container, calls } = makeView();
        installViewInput(view, { wheelZoom: () => true, gestureIdleMs: 1000 });
        fire(container, 'wheel', { deltaY: -30 });
        fire(container, 'wheel', { deltaY: -100 });
        assert.deepEqual(calls.map((c) => c.length), [4, 3, 3]);
        assert.equal(view.zoom, 5);
    });
});

describe('keyboard', () => {
    it('pans by the arrow step, three times as far with Shift', () => {
        assert.deepEqual(arrowPanOffset('ArrowRight', 80, false), [80, 0]);
        assert.deepEqual(arrowPanOffset('ArrowUp', 80, true), [0, -240]);
        assert.equal(arrowPanOffset('x', 80, false), null);
    });

    it('pans the view on an arrow key', () => {
        const { view, container, calls } = makeView();
        installViewInput(view, { arrowStep: () => 50 });
        fire(container, 'keydown', { key: 'ArrowDown' });
        assert.deepEqual(calls[0].slice(0, 3), ['pan', 0, 50]);
    });

    it('leaves modified keys to the rest of the app', () => {
        const { view, container, calls } = makeView();
        installViewInput(view);
        fire(container, 'keydown', { key: 'ArrowDown', ctrlKey: true });
        assert.equal(calls.length, 0);
    });

    it('zooms on + and - about the centre, not in a saved report', () => {
        const { view, container, calls } = makeView();
        let isStatic = false;
        installViewInput(view, { isStatic: () => isStatic });
        fire(container, 'keydown', { key: '+' });
        fire(container, 'keydown', { key: '-' });
        isStatic = true;
        fire(container, 'keydown', { key: '+' });
        assert.deepEqual(calls, [['zoom', null, 1], ['zoom', null, -1]]);
    });
});

describe('left button', () => {
    it('reports a click that moved within the tolerance', () => {
        const { view, container } = makeView();
        const clicks = [];
        installViewInput(view, { onClick: (w, e) => clicks.push([w, e.shiftKey]) });
        fire(container, 'pointerdown', { button: 0, clientX: 10, clientY: 10 });
        fire(window, 'pointermove', { clientX: 12, clientY: 11 });
        fire(window, 'pointerup', { button: 0, clientX: 12, clientY: 11,
                                    shiftKey: true });
        assert.deepEqual(clicks, [[[6, 5.5], true]]);
    });

    // A label handle or ruler label handles its own clicks and drags.
    it('ignores presses that start on a marker', () => {
        const { view, container, calls } = makeView();
        const clicks = [];
        installViewInput(view, { onClick: () => clicks.push(1) });
        const marker = document.createElement('div');
        marker.className = 'or-view-marker';
        container.appendChild(marker);
        fire(marker, 'pointerdown', { button: 0, clientX: 5, clientY: 5 });
        fire(window, 'pointerup', { button: 0, clientX: 5, clientY: 5 });
        fire(marker, 'dblclick', {});
        assert.equal(clicks.length, 0);
        assert.equal(calls.length, 0);
    });

    // The zoom buttons, HUD and legend sit inside the view's container.
    it('leaves presses on a control over the layout to the control', () => {
        const { view, container, calls } = makeView();
        const clicks = [];
        const context = [];
        installViewInput(view, { onClick: () => clicks.push(1),
                                 onContextClick: () => context.push(1) });
        const button = document.createElement('button');
        container.appendChild(button);
        fire(button, 'pointerdown', { button: 0, clientX: 5, clientY: 5 });
        fire(window, 'pointerup', { button: 0, clientX: 5, clientY: 5 });
        fire(button, 'dblclick', { clientX: 5, clientY: 5 });
        fire(button, 'pointerdown', { button: 2, clientX: 5, clientY: 5 });
        fire(window, 'pointerup', { button: 2, clientX: 5, clientY: 5 });
        assert.equal(clicks.length, 0);
        assert.equal(context.length, 0);
        assert.equal(calls.length, 0);
    });

    it('reports a click on the canvas the renderer draws in', () => {
        const { view, container } = makeView();
        const clicks = [];
        installViewInput(view, { onClick: () => clicks.push(1) });
        const wrap = document.createElement('div');
        wrap.className = 'or-view-canvas';
        const canvas = document.createElement('canvas');
        wrap.appendChild(canvas);
        container.appendChild(wrap);
        fire(canvas, 'pointerdown', { button: 0, clientX: 5, clientY: 5 });
        fire(window, 'pointerup', { button: 0, clientX: 5, clientY: 5 });
        assert.equal(clicks.length, 1);
    });

    it('does not report a drag as a click', () => {
        const { view, container } = makeView();
        const clicks = [];
        installViewInput(view, { onClick: () => clicks.push(1) });
        fire(container, 'pointerdown', { button: 0, clientX: 10, clientY: 10 });
        fire(window, 'pointermove', { clientX: 30, clientY: 10 });
        fire(window, 'pointerup', { button: 0, clientX: 30, clientY: 10 });
        assert.equal(clicks.length, 0);
    });

    // Shift+drag zooms to the box and must not pan.
    it('zooms to a Shift+drag box without letting the renderer pan', () => {
        const { view, container, calls } = makeView();
        const wrap = document.createElement('div');
        wrap.className = 'or-view-canvas';
        const canvas = document.createElement('canvas');
        wrap.appendChild(canvas);
        container.appendChild(wrap);
        let rendererSaw = 0;
        canvas.addEventListener('pointerdown', () => rendererSaw++);
        installViewInput(view);
        fire(canvas, 'pointerdown', { button: 0, shiftKey: true,
                                      clientX: 10, clientY: 10 });
        fire(window, 'pointermove', { clientX: 50, clientY: 30 });
        assert.ok(document.querySelector('.rubber-band'));
        fire(window, 'pointerup', { button: 0, clientX: 50, clientY: 30 });
        assert.equal(rendererSaw, 0);
        assert.deepEqual(calls, [['fit', [5, 5, 25, 15]]]);
        assert.equal(document.querySelector('.rubber-band'), null);
    });

    function shiftCanvas(container) {
        const wrap = document.createElement('div');
        wrap.className = 'or-view-canvas';
        const canvas = document.createElement('canvas');
        wrap.appendChild(canvas);
        container.appendChild(wrap);
        return canvas;
    }

    // A Shift+click adds to the selection; a hand that moves a few pixels on
    // one axis must not turn it into a zoom to a sliver.
    it('keeps a Shift+click that jitters on one axis a click', () => {
        const { view, container, calls } = makeView();
        const clicks = [];
        installViewInput(view, { onClick: (w, e) => clicks.push([w, e.shiftKey]) });
        const canvas = shiftCanvas(container);
        fire(canvas, 'pointerdown', { button: 0, shiftKey: true,
                                      clientX: 10, clientY: 10 });
        fire(window, 'pointermove', { clientX: 16, clientY: 10 });
        assert.equal(document.querySelector('.rubber-band'), null);
        fire(window, 'pointerup', { button: 0, shiftKey: true,
                                    clientX: 16, clientY: 10 });
        assert.deepEqual(calls, []);
        assert.deepEqual(clicks, [[[8, 5], true]]);
    });

    it('does nothing for a long Shift+drag along one axis', () => {
        const { view, container, calls } = makeView();
        const clicks = [];
        installViewInput(view, { onClick: (w, e) => clicks.push([w, e.shiftKey]) });
        const canvas = shiftCanvas(container);
        fire(canvas, 'pointerdown', { button: 0, shiftKey: true,
                                      clientX: 10, clientY: 10 });
        fire(window, 'pointermove', { clientX: 310, clientY: 12 });
        assert.equal(document.querySelector('.rubber-band'), null);
        fire(window, 'pointerup', { button: 0, shiftKey: true,
                                    clientX: 310, clientY: 12 });
        assert.deepEqual(calls, []);
        assert.deepEqual(clicks, []);
    });

    it('drops a band the browser takes away', () => {
        for (const [type, target] of [['pointercancel', window], ['blur', window]]) {
            const { view, container, calls } = makeView();
            const input = installViewInput(view);
            const canvas = shiftCanvas(container);
            fire(canvas, 'pointerdown', { button: 0, shiftKey: true,
                                          clientX: 10, clientY: 10 });
            fire(window, 'pointermove', { clientX: 50, clientY: 30 });
            fire(container, 'pointerdown', { button: 2, clientX: 100, clientY: 80 });
            fire(window, 'pointermove', { clientX: 60, clientY: 20 });
            assert.equal(document.querySelectorAll('.rubber-band').length, 2, type);
            fire(target, type);
            assert.equal(document.querySelector('.rubber-band'), null, type);
            fire(window, 'pointerup', { button: 0, clientX: 50, clientY: 30 });
            fire(window, 'pointerup', { button: 2, clientX: 60, clientY: 20 });
            assert.deepEqual(calls, [], type);
            input.dispose();
        }
    });

    it('removes a band in flight when disposed', () => {
        const { view, container } = makeView();
        const input = installViewInput(view);
        fire(shiftCanvas(container), 'pointerdown', { button: 0, shiftKey: true,
                                                     clientX: 10, clientY: 10 });
        fire(window, 'pointermove', { clientX: 50, clientY: 30 });
        assert.ok(document.querySelector('.rubber-band'));
        input.dispose();
        assert.equal(document.querySelector('.rubber-band'), null);
    });

    it('zooms in on a double click, out with Shift', () => {
        const { view, container, calls } = makeView();
        installViewInput(view);
        fire(container, 'dblclick', { clientX: 8, clientY: 4 });
        fire(container, 'dblclick', { clientX: 8, clientY: 4, shiftKey: true });
        assert.deepEqual(calls, [['zoom', [8, 4], 1], ['zoom', [8, 4], -1]]);
    });
});

describe('right button', () => {
    it('zooms to a right-drag box', () => {
        const { view, container, calls } = makeView();
        installViewInput(view);
        fire(container, 'pointerdown', { button: 2, clientX: 100, clientY: 80 });
        fire(window, 'pointermove', { clientX: 60, clientY: 20 });
        fire(window, 'pointerup', { button: 2, clientX: 60, clientY: 20 });
        assert.deepEqual(calls, [['fit', [30, 10, 50, 40]]]);
    });

    it('opens the context menu on a right click', () => {
        const { view, container } = makeView();
        const context = [];
        installViewInput(view, { onContextClick: (w) => context.push(w) });
        const menu = fire(container, 'contextmenu', { button: 2 });
        assert.ok(menu.defaultPrevented);
        fire(container, 'pointerdown', { button: 2, clientX: 4, clientY: 2 });
        fire(window, 'pointerup', { button: 2, clientX: 5, clientY: 3 });
        assert.deepEqual(context, [[2.5, 1.5]]);
    });

    it('skips the context menu while a ruler is placed', () => {
        const { view, container } = makeView();
        const context = [];
        installViewInput(view, { onContextClick: () => context.push(1),
                                 contextEnabled: () => false });
        fire(container, 'pointerdown', { button: 2, clientX: 4, clientY: 2 });
        fire(window, 'pointerup', { button: 2, clientX: 4, clientY: 2 });
        assert.equal(context.length, 0);
    });
});

describe('pointer tracking', () => {
    it('reports moves in world units and leaving the view', () => {
        const { view, container } = makeView();
        const moves = [];
        let left = 0;
        installViewInput(view, { onMove: (w) => moves.push(w),
                                 onLeave: () => left++ });
        fire(container, 'pointermove', { clientX: 20, clientY: 40 });
        fire(container, 'pointerleave', {});
        assert.deepEqual(moves, [[10, 20]]);
        assert.equal(left, 1);
    });

    it('stops listening once disposed', () => {
        const { view, container, calls } = makeView();
        const input = installViewInput(view);
        input.dispose();
        fire(container, 'dblclick', {});
        assert.equal(calls.length, 0);
    });
});
