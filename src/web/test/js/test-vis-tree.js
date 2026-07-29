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
    let visibility, selectability, changes, tree;

    beforeEach(() => {
        visibility = {};
        selectability = {};
        changes = 0;
        tree = new VisTree(visibility, selectability, () => { changes++; });
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

    describe('pin visibility with disabledBy', () => {
        it('pin_names is grayed when pins is off', () => {
            visibility.pins = false;
            visibility.pin_names = true;
            tree.add({ label: 'Group', children: [
                { key: 'pins', label: 'Pins' },
                { key: 'pin_names', label: 'Pin Names', disabledBy: 'pins' },
            ]});
            const container = document.createElement('div');
            tree.render(container);
            assert.equal(visibility.pins, false);
            // pin_names value preserved but its label should be disabled.
            const labels = container.querySelectorAll('label');
            const pinNamesLabel = [...labels].find(l => l.textContent.includes('Pin Names'));
            assert.ok(pinNamesLabel.classList.contains('disabled'));
        });

        it('pin_names is enabled when pins is on', () => {
            visibility.pins = true;
            visibility.pin_names = true;
            tree.add({ label: 'Group', children: [
                { key: 'pins', label: 'Pins' },
                { key: 'pin_names', label: 'Pin Names', disabledBy: 'pins' },
            ]});
            const container = document.createElement('div');
            tree.render(container);
            const labels = container.querySelectorAll('label');
            const pinNamesLabel = [...labels].find(l => l.textContent.includes('Pin Names'));
            assert.ok(!pinNamesLabel.classList.contains('disabled'));
        });

        it('toggling pins updates pin_names disabled state', () => {
            visibility.pins = true;
            visibility.pin_names = true;
            tree.add({ label: 'Group', children: [
                { key: 'pins', label: 'Pins' },
                { key: 'pin_names', label: 'Pin Names', disabledBy: 'pins' },
            ]});
            const container = document.createElement('div');
            tree.render(container);

            // Uncheck pins.
            const labels = container.querySelectorAll('label');
            const pinsLabel = [...labels].find(l =>
                l.textContent.includes('Pins') && !l.textContent.includes('Names'));
            const pinsCb = pinsLabel.querySelector('input');
            pinsCb.checked = false;
            pinsCb.dispatchEvent(new dom.window.Event('change'));

            assert.equal(visibility.pins, false);
            const pinNamesLabel = [...labels].find(l => l.textContent.includes('Pin Names'));
            assert.ok(pinNamesLabel.classList.contains('disabled'));
        });
    });

    describe('selectability column', () => {
        it('no second checkbox when addSelectable not set', () => {
            visibility.a = true;
            tree.add({ label: 'Group', children: [
                { key: 'a', label: 'A' },
            ]});
            const container = document.createElement('div');
            tree.render(container);
            // Only visibility checkbox on the leaf (plus parent header cb).
            const leafLabel = container.querySelector('.vis-group-children label');
            assert.equal(leafLabel.querySelectorAll('input[type=checkbox]').length, 1);
            // selectability state untouched for keys without selectable column.
            assert.equal(selectability.a, undefined);
        });

        it('renders second checkbox on group when addSelectable is true', () => {
            visibility.a = true;
            tree.add({ label: 'Group', addSelectable: true, children: [
                { key: 'a', label: 'A' },
            ]});
            const container = document.createElement('div');
            tree.render(container);
            const leafLabel = container.querySelector('.vis-group-children label');
            const inputs = leafLabel.querySelectorAll('input[type=checkbox]');
            assert.equal(inputs.length, 2);
            // The header (group) also gets a selectability checkbox.
            const headerInputs = container.querySelectorAll(
                '.vis-group-header input[type=checkbox]');
            assert.equal(headerInputs.length, 2);
        });

        it('toggling selectability checkbox updates selectability object', () => {
            visibility.a = true;
            selectability.a = true;
            tree.add({ label: 'Group', addSelectable: true, children: [
                { key: 'a', label: 'A' },
            ]});
            const container = document.createElement('div');
            tree.render(container);
            const leafLabel = container.querySelector('.vis-group-children label');
            const [, selCb] = leafLabel.querySelectorAll('input[type=checkbox]');
            selCb.checked = false;
            selCb.dispatchEvent(new dom.window.Event('change'));
            assert.equal(selectability.a, false);
        });

        it('parent selectability checkbox tri-states from children', () => {
            visibility.a = true;
            visibility.b = true;
            selectability.a = true;
            selectability.b = true;
            tree.add({ label: 'Group', addSelectable: true, children: [
                { key: 'a', label: 'A' },
                { key: 'b', label: 'B' },
            ]});
            const container = document.createElement('div');
            tree.render(container);
            // Uncheck child A's selectability.
            const leaves = container.querySelectorAll('.vis-group-children label');
            const [, aSel] = leaves[0].querySelectorAll('input[type=checkbox]');
            aSel.checked = false;
            aSel.dispatchEvent(new dom.window.Event('change'));
            // Parent header sel checkbox becomes indeterminate.
            const headerInputs = container.querySelectorAll(
                '.vis-group-header input[type=checkbox]');
            const parentSel = headerInputs[1];
            assert.equal(parentSel.indeterminate, true);
        });

        it('unchecking visibility disables selectability checkbox', () => {
            visibility.a = true;
            selectability.a = true;
            tree.add({ label: 'Group', addSelectable: true, children: [
                { key: 'a', label: 'A' },
            ]});
            const container = document.createElement('div');
            tree.render(container);
            const leafLabel = container.querySelector('.vis-group-children label');
            const [visCb, selCb] = leafLabel.querySelectorAll(
                'input[type=checkbox]');
            assert.equal(selCb.disabled, false);
            visCb.checked = false;
            visCb.dispatchEvent(new dom.window.Event('change'));
            assert.equal(selCb.disabled, true);
        });

        it('toggling parent propagates to all selectable descendants', () => {
            visibility.a = true;
            visibility.b = true;
            visibility.c = true;
            selectability.a = true;
            selectability.b = true;
            selectability.c = true;
            tree.add({ label: 'Group', addSelectable: true, children: [
                { label: 'Sub', children: [
                    { key: 'a', label: 'A' },
                    { key: 'b', label: 'B' },
                ]},
                { key: 'c', label: 'C' },
            ]});
            const container = document.createElement('div');
            tree.render(container);
            // The outer "Group" header has [vis, sel] checkboxes.
            const headerInputs = container.querySelectorAll(
                '.vis-group-header')[0].querySelectorAll(
                'input[type=checkbox]');
            const groupSel = headerInputs[1];
            groupSel.checked = false;
            groupSel.dispatchEvent(new dom.window.Event('change'));
            assert.equal(selectability.a, false);
            assert.equal(selectability.b, false);
            assert.equal(selectability.c, false);
        });

        it('same-labeled groups under different parents stay independent', () => {
            // Regression: two "Instances" groups (top-level and under Misc)
            // used to collide on the model node id, so toggling the parent
            // resolved to the wrong subtree.
            visibility.a = true;
            visibility.b = true;
            selectability.a = true;
            selectability.b = true;
            tree.add({ label: 'Instances', addSelectable: true, children: [
                { key: 'a', label: 'A' },
            ]});
            tree.add({ label: 'Misc', children: [
                { label: 'Instances', children: [
                    { key: 'b', label: 'B', selectable: true },
                ]},
            ]});
            const container = document.createElement('div');
            tree.render(container);
            // The first '.vis-group-header' belongs to the top-level Instances.
            const topHeader = container.querySelectorAll(
                '.vis-group-header')[0];
            const [, topSel] = topHeader.querySelectorAll(
                'input[type=checkbox]');
            topSel.checked = false;
            topSel.dispatchEvent(new dom.window.Event('change'));
            assert.equal(selectability.a, false);
            // The unrelated Misc/Instances leaf should be untouched.
            assert.equal(selectability.b, true);
        });

        it('leaf with selectable: true opts in without addSelectable', () => {
            visibility.a = true;
            selectability.a = true;
            tree.add({ label: 'Group', children: [
                { key: 'a', label: 'A', selectable: true },
                { key: 'b', label: 'B' },
            ]});
            const container = document.createElement('div');
            tree.render(container);
            const leaves = container.querySelectorAll('.vis-group-children label');
            assert.equal(
                leaves[0].querySelectorAll('input[type=checkbox]').length, 2);
            assert.equal(
                leaves[1].querySelectorAll('input[type=checkbox]').length, 1);
        });
    });

});
