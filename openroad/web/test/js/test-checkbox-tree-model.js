// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it, beforeEach } from 'node:test';
import assert from 'node:assert/strict';

const { CheckboxTreeModel } = await import('../../src/checkbox-tree-model.js');

describe('CheckboxTreeModel', () => {
    let changes, model;

    beforeEach(() => {
        changes = 0;
        model = new CheckboxTreeModel(() => { changes++; });
    });

    // -- addFromSpec --

    describe('addFromSpec', () => {
        it('single leaf defaults to checked', () => {
            model.addFromSpec({ id: 'a' });
            assert.equal(model.get('a').checked, true);
        });

        it('single leaf respects checked:false', () => {
            model.addFromSpec({ id: 'a', checked: false });
            assert.equal(model.get('a').checked, false);
        });

        it('parent checked when all children checked', () => {
            model.addFromSpec({ id: 'p', children: [
                { id: 'a', checked: true },
                { id: 'b', checked: true },
            ]});
            assert.equal(model.get('p').checked, true);
            assert.equal(model.get('p').indeterminate, false);
        });

        it('parent unchecked when all children unchecked', () => {
            model.addFromSpec({ id: 'p', children: [
                { id: 'a', checked: false },
                { id: 'b', checked: false },
            ]});
            assert.equal(model.get('p').checked, false);
            assert.equal(model.get('p').indeterminate, false);
        });

        it('parent indeterminate when children mixed', () => {
            model.addFromSpec({ id: 'p', children: [
                { id: 'a', checked: true },
                { id: 'b', checked: false },
            ]});
            assert.equal(model.get('p').checked, false);
            assert.equal(model.get('p').indeterminate, true);
        });

        it('three levels deep', () => {
            model.addFromSpec({ id: 'root', children: [
                { id: 'g1', children: [
                    { id: 'a', checked: true },
                    { id: 'b', checked: false },
                ]},
                { id: 'g2', children: [
                    { id: 'c', checked: true },
                    { id: 'd', checked: true },
                ]},
            ]});
            assert.equal(model.get('g1').indeterminate, true);
            assert.equal(model.get('g2').checked, true);
            assert.equal(model.get('root').indeterminate, true);
        });
    });

    // -- check --

    describe('check', () => {
        it('unchecking leaf updates parent', () => {
            model.addFromSpec({ id: 'p', children: [
                { id: 'a', checked: true },
                { id: 'b', checked: true },
            ]});
            model.check('a', false);
            assert.equal(model.get('a').checked, false);
            assert.equal(model.get('p').indeterminate, true);
            assert.equal(changes, 1);
        });

        it('unchecking all children unchecks parent', () => {
            model.addFromSpec({ id: 'p', children: [
                { id: 'a', checked: true },
                { id: 'b', checked: true },
            ]});
            model.check('a', false);
            model.check('b', false);
            assert.equal(model.get('p').checked, false);
            assert.equal(model.get('p').indeterminate, false);
        });

        it('checking parent cascades to children', () => {
            model.addFromSpec({ id: 'p', children: [
                { id: 'a', checked: false },
                { id: 'b', checked: false },
            ]});
            model.check('p', true);
            assert.equal(model.get('a').checked, true);
            assert.equal(model.get('b').checked, true);
        });

        it('unchecking parent cascades to children', () => {
            model.addFromSpec({ id: 'p', children: [
                { id: 'a', checked: true },
                { id: 'b', checked: true },
            ]});
            model.check('p', false);
            assert.equal(model.get('a').checked, false);
            assert.equal(model.get('b').checked, false);
        });

        it('deep propagation: unchecking grandchild updates grandparent', () => {
            model.addFromSpec({ id: 'root', children: [
                { id: 'g', children: [
                    { id: 'a', checked: true },
                    { id: 'b', checked: true },
                ]},
            ]});
            model.check('a', false);
            assert.equal(model.get('g').indeterminate, true);
            assert.equal(model.get('root').indeterminate, true);
        });
    });

    // -- hasCheckbox:false --

    describe('hasCheckbox:false', () => {
        it('non-checkbox children skipped in parent computation', () => {
            model.addFromSpec({ id: 'p', children: [
                { id: 'structural', hasCheckbox: false },
                { id: 'a', checked: true },
            ]});
            // Parent state based only on 'a', not 'structural'
            assert.equal(model.get('p').checked, true);
            assert.equal(model.get('p').indeterminate, false);
        });

        it('setSubtree traverses through non-checkbox nodes', () => {
            // p -> structural (no cb) -> a (cb)
            model.addFromSpec({ id: 'p', children: [
                { id: 'structural', hasCheckbox: false, children: [
                    { id: 'a', checked: true },
                ]},
            ]});
            model.check('p', false);
            assert.equal(model.get('a').checked, false);
        });

        it('unchecking deep child through non-checkbox updates parent', () => {
            // p -> structural (no cb) -> a (cb)
            model.addFromSpec({ id: 'p', children: [
                { id: 'structural', hasCheckbox: false, children: [
                    { id: 'a', checked: true },
                ]},
                { id: 'b', checked: true },
            ]});
            model.check('a', false);
            assert.equal(model.get('p').indeterminate, true);
        });
    });

    // -- buildFromNodes --

    describe('buildFromNodes', () => {
        it('builds tree from flat list', () => {
            model.buildFromNodes([
                { id: 0, parentId: -1, checked: true },
                { id: 1, parentId: 0, checked: true },
                { id: 2, parentId: 0, checked: false },
            ]);
            assert.equal(model.roots.length, 1);
            assert.equal(model.get(0).children.length, 2);
            assert.equal(model.get(0).indeterminate, true);
        });

        it('check works after buildFromNodes', () => {
            model.buildFromNodes([
                { id: 0, parentId: -1, checked: true },
                { id: 1, parentId: 0, checked: true },
                { id: 2, parentId: 0, checked: true },
            ]);
            model.check(1, false);
            assert.equal(model.get(0).indeterminate, true);
        });

        it('hasCheckbox:false in flat list', () => {
            model.buildFromNodes([
                { id: 0, parentId: -1, checked: true },       // module
                { id: 1, parentId: 0, hasCheckbox: false },    // leaf group
                { id: 2, parentId: 0, checked: true },         // module
                { id: 3, parentId: 0, checked: false },        // module
            ]);
            // Parent computed from children 2 and 3 only (1 has no checkbox)
            assert.equal(model.get(0).indeterminate, true);
        });
    });

    // -- checkSet --

    describe('checkSet', () => {
        it('bulk update with single onChange', () => {
            model.addFromSpec({ id: 'p', children: [
                { id: 'a', checked: true },
                { id: 'b', checked: true },
                { id: 'c', checked: true },
            ]});
            changes = 0;
            model.checkSet({ a: false, b: false });
            assert.equal(changes, 1);
            assert.equal(model.get('a').checked, false);
            assert.equal(model.get('b').checked, false);
            assert.equal(model.get('c').checked, true);
            assert.equal(model.get('p').indeterminate, true);
        });

        it('uncheck all via checkSet', () => {
            model.addFromSpec({ id: 'p', children: [
                { id: 'a', checked: true },
                { id: 'b', checked: true },
            ]});
            model.checkSet({ a: false, b: false });
            assert.equal(model.get('p').checked, false);
            assert.equal(model.get('p').indeterminate, false);
        });
    });

    // -- forEach --

    describe('forEach', () => {
        it('visits all nodes in DFS order', () => {
            model.addFromSpec({ id: 'p', children: [
                { id: 'a' },
                { id: 'b' },
            ]});
            const ids = [];
            model.forEach(n => ids.push(n.id));
            assert.deepEqual(ids, ['p', 'a', 'b']);
        });
    });
});
