// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it, beforeEach } from 'node:test';
import assert from 'node:assert/strict';
import { JSDOM } from 'jsdom';

// Set up minimal DOM before importing vis-tree.
const dom = new JSDOM('<!DOCTYPE html><html><body></body></html>');
globalThis.document = dom.window.document;

const { VisTree, makeColumnHeader, makeNameSpan }
    = await import('../../src/vis-tree.js');

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

    describe('group header clicks', () => {
        const click = (el) => el.dispatchEvent(
            new dom.window.MouseEvent('click', { bubbles: true }));

        // Build a one-group tree with both children visible.  The container
        // is attached to the document because jsdom only runs a checkbox's
        // activation behavior (toggle + `change`) for connected elements.
        const build = () => {
            visibility.a = true;
            visibility.b = true;
            tree.add({ label: 'Group', children: [
                { key: 'a', label: 'A' },
                { key: 'b', label: 'B' },
            ]});
            const container = document.createElement('div');
            document.body.textContent = '';
            document.body.appendChild(container);
            tree.render(container);
            return {
                header: container.querySelector('.vis-group-header'),
                arrow: container.querySelector('.vis-group-header .vis-arrow'),
                name: container.querySelector('.vis-group-header .vis-name'),
                kids: container.querySelector('.vis-group-children'),
                cb: container.querySelector('.vis-group-header input'),
            };
        };

        it('starts collapsed', () => {
            const { arrow, kids } = build();
            assert.ok(kids.classList.contains('collapsed'));
            assert.equal(arrow.textContent, '▶');
        });

        it('clicking the arrow expands and collapses', () => {
            const { arrow, kids } = build();
            click(arrow);
            assert.ok(!kids.classList.contains('collapsed'));
            assert.equal(arrow.textContent, '▼');
            click(arrow);
            assert.ok(kids.classList.contains('collapsed'));
            assert.equal(arrow.textContent, '▶');
        });

        it('clicking the arrow does not change visibility', () => {
            const { arrow } = build();
            click(arrow);
            assert.equal(visibility.a, true);
            assert.equal(visibility.b, true);
        });

        // Regression: the header used to be a <label> wrapping the visibility
        // checkbox, so a click on the group name -- or one that merely missed
        // the small arrow glyph -- toggled the whole category's visibility.
        it('clicking the group name expands instead of toggling visibility',
           () => {
               const { header, kids } = build();
               click(header);
               assert.ok(!kids.classList.contains('collapsed'));
               assert.equal(visibility.a, true);
               assert.equal(visibility.b, true);
           });

        // The group name is a .vis-name span (it has to be an element so it
        // can stretch and push the checkbox columns right).  It used to be a
        // bare text node whose clicks reported the header as the target, so
        // wrapping it would silently drop the row's largest click target
        // unless attachGroupCollapse treats the span as a target too.
        it('clicking the name cell expands instead of toggling visibility',
           () => {
               const { name, kids } = build();
               assert.equal(name.textContent, 'Group');
               click(name);
               assert.ok(!kids.classList.contains('collapsed'));
               assert.equal(visibility.a, true);
               assert.equal(visibility.b, true);
               click(name);
               assert.ok(kids.classList.contains('collapsed'));
           });

        it('clicking the visibility checkbox does not expand', () => {
            const { cb, kids } = build();
            click(cb);
            assert.ok(kids.classList.contains('collapsed'));
            assert.equal(visibility.a, false);
            assert.equal(visibility.b, false);
        });

        // Only the triangle and the header's own bare area collapse, so a
        // control added to the row acts without also collapsing the group.
        it('clicking a control added to the header does not expand', () => {
            const { header, kids } = build();
            const extra = document.createElement('label');
            const extraCb = document.createElement('input');
            extraCb.type = 'checkbox';
            extra.appendChild(extraCb);
            extra.appendChild(document.createTextNode('Extra'));
            header.appendChild(extra);

            click(extra);
            assert.equal(extraCb.checked, true);
            assert.ok(kids.classList.contains('collapsed'));
        });
    });

    // Leaf rows have the same rule as group headers: only the checkboxes
    // change state.  They used to be <label>s wrapping the visibility
    // checkbox, so a click on the name, on the indent spacer, or anywhere in
    // the row's padding flipped the item — the box changed when the user had
    // aimed at the text.
    describe('leaf row clicks', () => {
        const click = (el) => el.dispatchEvent(
            new dom.window.MouseEvent('click', { bubbles: true }));

        const build = () => {
            visibility.a = true;
            tree.add({ label: 'Group', children: [{ key: 'a', label: 'A' }] });
            const container = document.createElement('div');
            // jsdom only runs a control's activation behavior when connected.
            document.body.textContent = '';
            document.body.appendChild(container);
            tree.render(container);
            const row = container.querySelector('.vis-leaf');
            return {
                row,
                name: row.querySelector('.vis-name'),
                spacer: row.querySelector('.vis-arrow'),
                cb: row.querySelector('input.vis-cb'),
            };
        };

        it('is not a label, so nothing in it implicitly toggles', () => {
            const { row } = build();
            assert.equal(row.tagName, 'DIV');
        });

        it('clicking the leaf name does not toggle visibility', () => {
            const { name, cb } = build();
            click(name);
            assert.equal(cb.checked, true);
            assert.equal(visibility.a, true);
        });

        it('clicking the row itself does not toggle visibility', () => {
            const { row, cb } = build();
            click(row);
            assert.equal(cb.checked, true);
            assert.equal(visibility.a, true);
        });

        it('clicking the indent spacer does not toggle visibility', () => {
            const { spacer, cb } = build();
            click(spacer);
            assert.equal(cb.checked, true);
            assert.equal(visibility.a, true);
        });

        it('clicking the checkbox still toggles visibility', () => {
            const { cb } = build();
            click(cb);
            assert.equal(cb.checked, false);
            assert.equal(visibility.a, false);
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
            const labels = container.querySelectorAll('.vis-leaf');
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
            const labels = container.querySelectorAll('.vis-leaf');
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
            const labels = container.querySelectorAll('.vis-leaf');
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
            const leafLabel = container.querySelector('.vis-group-children .vis-leaf');
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
            const leafLabel = container.querySelector('.vis-group-children .vis-leaf');
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
            const leafLabel = container.querySelector('.vis-group-children .vis-leaf');
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
            const leaves = container.querySelectorAll('.vis-group-children .vis-leaf');
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
            const leafLabel = container.querySelector('.vis-group-children .vis-leaf');
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
            const leaves = container.querySelectorAll('.vis-group-children .vis-leaf');
            assert.equal(
                leaves[0].querySelectorAll('input[type=checkbox]').length, 2);
            assert.equal(
                leaves[1].querySelectorAll('input[type=checkbox]').length, 1);
        });
    });

    // Column layout mirrors the Qt GUI (displayControls.cpp): the name cell
    // stretches and the visibility / selectability checkboxes are the last two
    // elements of every row, so they form two straight columns under the
    // header icons.  Anything appended after them (e.g. a bare label text
    // node, as the rows used to do) breaks that alignment.
    describe('column layout', () => {
        // Every row (group headers and leaves alike) must end with the
        // visibility column then the selectability column, and carry its text
        // in a .vis-name cell before them.
        function checkRow(row) {
            const cells = row.children;
            assert.ok(cells.length >= 3,
                      `row "${row.textContent}" has too few cells`);
            const vis = cells[cells.length - 2];
            const sel = cells[cells.length - 1];
            assert.ok(vis.classList.contains('vis-cb'),
                      `expected vis column last-but-one, got ${vis.className}`);
            assert.ok(sel.classList.contains('vis-sel-cb'),
                      `expected sel column last, got ${sel.className}`);
            assert.equal(vis.tagName, 'INPUT');
            assert.equal(vis.type, 'checkbox');
            const name = row.querySelector('.vis-name');
            assert.ok(name, `row "${row.textContent}" has no .vis-name cell`);
            // No stray text outside the name cell — that is what used to sit
            // to the right of the checkboxes.
            for (const node of row.childNodes) {
                if (node.nodeType === 3) {
                    assert.equal(node.textContent.trim(), '',
                                 'row text must live in the .vis-name cell');
                }
            }
        }

        function renderSample() {
            visibility.a = true;
            tree.add({ label: 'Nets', addSelectable: true, children: [
                { key: 'a', label: 'Signal' },
                { key: 'b', label: 'Power' },
            ]});
            tree.add({ label: 'Tracks', children: [
                { key: 'c', label: 'Pref' },
            ]});
            tree.add({ key: 'd', label: 'Module view' });
            const container = document.createElement('div');
            tree.render(container);
            return container;
        }

        it('checkboxes are the last two cells of every row', () => {
            const container = renderSample();
            const rows = container.querySelectorAll(
                '.vis-group-header, .vis-leaf');
            assert.ok(rows.length >= 6, `only ${rows.length} rows rendered`);
            rows.forEach(checkRow);
        });

        it('rows without selectability keep a spacer in that column', () => {
            const container = renderSample();
            // "Tracks" opts out of selectability, so its rows get the
            // layout-only placeholder rather than nothing at all.
            const rows = Array.from(container.querySelectorAll(
                '.vis-group-header, .vis-leaf'));
            const tracks = rows.find(r => r.textContent.includes('Pref'));
            const last = tracks.children[tracks.children.length - 1];
            assert.ok(last.classList.contains('vis-sel-spacer'));
            assert.equal(last.tagName, 'SPAN');
        });

        it('name text lives in the stretching name cell', () => {
            const container = renderSample();
            const rows = Array.from(container.querySelectorAll('.vis-leaf'));
            const signal = rows.find(r => r.textContent.includes('Signal'));
            assert.equal(signal.querySelector('.vis-name').textContent,
                         'Signal');
        });

        it('makeNameSpan builds a .vis-name cell', () => {
            const span = makeNameSpan('Metal1');
            assert.equal(span.className, 'vis-name');
            assert.equal(span.textContent, 'Metal1');
        });
    });

    // The panel header labels the two checkbox columns with the same icons the
    // Qt GUI uses in its QHeaderView (DisplayControlModel::headerData).
    describe('makeColumnHeader', () => {
        it('has a stretching name cell then two icon columns', () => {
            const header = makeColumnHeader();
            assert.equal(header.className, 'display-controls-header');
            assert.equal(header.children.length, 3);
            assert.ok(header.children[0].classList.contains('vis-name'));
            const icons = header.querySelectorAll('.vis-header-icon');
            assert.equal(icons.length, 2);
            assert.equal(icons[0].title, 'Visible');
            assert.equal(icons[1].title, 'Selectable');
        });

        it('renders an svg symbol in each column', () => {
            const header = makeColumnHeader();
            for (const icon of header.querySelectorAll('.vis-header-icon')) {
                assert.equal(icon.children.length, 1);
                assert.equal(icon.children[0].tagName.toLowerCase(), 'svg');
            }
        });
    });

});
