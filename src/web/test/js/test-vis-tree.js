// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it, beforeEach } from 'node:test';
import assert from 'node:assert/strict';
import { JSDOM } from 'jsdom';

// Set up minimal DOM before importing vis-tree.
const dom = new JSDOM('<!DOCTYPE html><html><body></body></html>');
globalThis.document = dom.window.document;

const { VisTree } = await import('../../src/vis-tree.js');

describe('VisTree', () => {
    let visibility, changes, tree;

    beforeEach(() => {
        visibility = {};
        changes = 0;
        tree = new VisTree(visibility, () => { changes++; });
    });

    describe('leaf nodes', () => {
        it('unchecked when key absent from visibility', () => {
            tree.add({ key: 'a', label: 'A' });
            tree.render(document.createElement('div'));
            assert.equal(visibility.a, false);
        });

        it('checked when visibility has key as true', () => {
            visibility.a = true;
            tree.add({ key: 'a', label: 'A' });
            tree.render(document.createElement('div'));
            assert.equal(visibility.a, true);
        });

        it('unchecked when visibility has key as false', () => {
            visibility.x = false;
            tree.add({ key: 'x', label: 'X' });
            tree.render(document.createElement('div'));
            assert.equal(visibility.x, false);
        });
    });

    describe('parent-child relationships', () => {
        it('parent checked when all children checked', () => {
            visibility.a = true;
            visibility.b = true;
            tree.add({ label: 'Group', children: [
                { key: 'a', label: 'A' },
                { key: 'b', label: 'B' },
            ]});
            const container = document.createElement('div');
            tree.render(container);
            const parentCb = container.querySelector('.vis-group-header input');
            assert.equal(parentCb.checked, true);
            assert.equal(parentCb.indeterminate, false);
        });

        it('parent unchecked when all children unchecked', () => {
            tree.add({ label: 'Group', children: [
                { key: 'a', label: 'A' },
                { key: 'b', label: 'B' },
            ]});
            const container = document.createElement('div');
            tree.render(container);
            const parentCb = container.querySelector('.vis-group-header input');
            assert.equal(parentCb.checked, false);
            assert.equal(parentCb.indeterminate, false);
        });

        it('unchecking parent unchecks all children', () => {
            visibility.a = true;
            visibility.b = true;
            tree.add({ label: 'Group', children: [
                { key: 'a', label: 'A' },
                { key: 'b', label: 'B' },
            ]});
            const container = document.createElement('div');
            tree.render(container);
            const parentCb = container.querySelector('.vis-group-header input');
            parentCb.checked = false;
            parentCb.dispatchEvent(new dom.window.Event('change'));
            assert.equal(visibility.a, false);
            assert.equal(visibility.b, false);
        });

        it('checking parent checks all children', () => {
            tree.add({ label: 'Group', children: [
                { key: 'a', label: 'A' },
                { key: 'b', label: 'B' },
            ]});
            const container = document.createElement('div');
            tree.render(container);
            const parentCb = container.querySelector('.vis-group-header input');
            parentCb.checked = true;
            parentCb.dispatchEvent(new dom.window.Event('change'));
            assert.equal(visibility.a, true);
            assert.equal(visibility.b, true);
        });

        it('parent becomes indeterminate when children mixed', () => {
            visibility.a = true;
            visibility.b = true;
            tree.add({ label: 'Group', children: [
                { key: 'a', label: 'A' },
                { key: 'b', label: 'B' },
            ]});
            const container = document.createElement('div');
            tree.render(container);
            // Uncheck just one child
            const childCbs = container.querySelectorAll('.vis-group-children input');
            childCbs[0].checked = false;
            childCbs[0].dispatchEvent(new dom.window.Event('change'));
            const parentCb = container.querySelector('.vis-group-header input');
            assert.equal(parentCb.indeterminate, true);
        });
    });

    describe('visKey', () => {
        it('sets visKey true when all children checked', () => {
            visibility.a = true;
            visibility.b = true;
            tree.add({ label: 'Group', visKey: 'group_vis', children: [
                { key: 'a', label: 'A' },
                { key: 'b', label: 'B' },
            ]});
            tree.render(document.createElement('div'));
            assert.equal(visibility.group_vis, true);
        });

        it('sets visKey true when some children checked (indeterminate)', () => {
            visibility.a = false;
            visibility.b = true;
            tree.add({ label: 'Group', visKey: 'group_vis', children: [
                { key: 'a', label: 'A' },
                { key: 'b', label: 'B' },
            ]});
            tree.render(document.createElement('div'));
            // indeterminate: visKey should still be true
            assert.equal(visibility.group_vis, true);
        });

        it('sets visKey false when no children checked', () => {
            tree.add({ label: 'Group', visKey: 'group_vis', children: [
                { key: 'a', label: 'A' },
                { key: 'b', label: 'B' },
            ]});
            tree.render(document.createElement('div'));
            assert.equal(visibility.group_vis, false);
        });
    });

    describe('onChange callback', () => {
        it('fires on checkbox change', () => {
            visibility.a = true;
            tree.add({ key: 'a', label: 'A' });
            const container = document.createElement('div');
            tree.render(container);
            const cb = container.querySelector('input');
            cb.checked = false;
            cb.dispatchEvent(new dom.window.Event('change'));
            assert.equal(changes, 1);
        });
    });

    describe('disabled groups', () => {
        it('marks children container as disabled', () => {
            tree.add({ label: 'Group', disabled: true, children: [
                { key: 'a', label: 'A' },
            ]});
            const container = document.createElement('div');
            tree.render(container);
            const kids = container.querySelector('.vis-group-children');
            assert.ok(kids.classList.contains('disabled'));
        });
    });
});
