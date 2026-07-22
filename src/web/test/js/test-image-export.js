// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it, beforeEach, afterEach } from 'node:test';
import assert from 'node:assert/strict';
import {
    downloadUrl,
    downloadCsv,
    svgToString,
} from '../../src/image-export.js';

describe('image-export', () => {
    let clicks;
    let objectUrls;
    let origCreate;
    let origRevoke;
    let origClick;

    const AnchorProto = window.HTMLAnchorElement.prototype;

    beforeEach(() => {
        document.body.innerHTML = '';
        clicks = [];
        objectUrls = [];
        // Capture anchor clicks instead of navigating.
        origClick = AnchorProto.click;
        AnchorProto.click = function () {
            clicks.push({ href: this.href, download: this.download });
        };
        // Stub blob URL plumbing (jsdom lacks it).
        origCreate = URL.createObjectURL;
        origRevoke = URL.revokeObjectURL;
        URL.createObjectURL = (blob) => {
            objectUrls.push(blob);
            return 'blob:mock/' + objectUrls.length;
        };
        URL.revokeObjectURL = () => {};
    });

    afterEach(() => {
        AnchorProto.click = origClick;
        URL.createObjectURL = origCreate;
        URL.revokeObjectURL = origRevoke;
    });

    it('downloadUrl creates an <a download> with the right href', () => {
        downloadUrl('data:text/plain,hi', 'note.txt');
        assert.equal(clicks.length, 1);
        assert.equal(clicks[0].download, 'note.txt');
        assert.ok(clicks[0].href.startsWith('data:text/plain'));
        // Anchor is cleaned up after clicking.
        assert.equal(document.querySelectorAll('a').length, 0);
    });

    it('downloadCsv builds RFC-4180 CSV with proper quoting', async () => {
        downloadCsv([
            ['lower', 'upper', 'count'],
            [-1.5, 0, 10],
            ['a,b', 'quote"x', 3],
        ], 'histogram.csv');

        assert.equal(clicks.length, 1);
        assert.equal(clicks[0].download, 'histogram.csv');
        assert.equal(objectUrls.length, 1);
        const blob = objectUrls[0];
        assert.match(blob.type, /text\/csv/);
        const text = await blob.text();
        const lines = text.split('\r\n');
        assert.equal(lines[0], 'lower,upper,count');
        assert.equal(lines[1], '-1.5,0,10');
        // Comma and quote get quoted; embedded quote is doubled.
        assert.equal(lines[2], '"a,b","quote""x",3');
    });

    it('svgToString injects xmlns and serializes the element', () => {
        const svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
        svg.setAttribute('width', '100');
        svg.setAttribute('height', '50');
        const rect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
        svg.appendChild(rect);

        const out = svgToString(svg);
        assert.ok(out.includes('<?xml'));
        assert.ok(out.includes('xmlns="http://www.w3.org/2000/svg"'));
        assert.ok(out.includes('<rect'));
    });
});
