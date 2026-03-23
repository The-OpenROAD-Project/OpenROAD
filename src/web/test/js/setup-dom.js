// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Minimal DOM environment for tests that need document/window.
import { JSDOM } from 'jsdom';
const dom = new JSDOM('<!DOCTYPE html><html><body></body></html>');
globalThis.document = dom.window.document;
globalThis.window = dom.window;
globalThis.Event = dom.window.Event;
export { dom };
