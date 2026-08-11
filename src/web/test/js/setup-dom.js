// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Minimal DOM environment for tests that need document/window.
import { JSDOM } from 'jsdom';
const dom = new JSDOM('<!DOCTYPE html><html><body></body></html>', {
    url: 'http://localhost/',
});
globalThis.document = dom.window.document;
globalThis.window = dom.window;
globalThis.Event = dom.window.Event;
globalThis.HTMLElement = dom.window.HTMLElement;
globalThis.HTMLCanvasElement = dom.window.HTMLCanvasElement;
globalThis.getComputedStyle = dom.window.getComputedStyle.bind(dom.window);
globalThis.localStorage = dom.window.localStorage;
globalThis.matchMedia = globalThis.matchMedia || (() => ({
    matches: false,
    addListener() {},
    removeListener() {},
    addEventListener() {},
    removeEventListener() {},
    dispatchEvent() { return false; },
}));
globalThis.ResizeObserver = globalThis.ResizeObserver || class {
    observe() {}
    disconnect() {}
};
// Let queued promise callbacks (a request's .then chain) run.  Every widget
// test needs this; it lived as an inline `new Promise(r => setTimeout(r, 0))`
// copied through the suites.
export function waitForMicrotasks() {
    return new Promise(resolve => setTimeout(resolve, 0));
}

export { dom };
