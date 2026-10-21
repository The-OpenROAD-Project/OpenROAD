// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it, beforeEach } from 'node:test';
import assert from 'node:assert/strict';
import { GridLayoutManager, IdePaneElement, IdeSplitterElement, IdeTabElement } from '../../src/grid-layout.js';

describe('GridLayoutManager', () => {
    let containerEl;
    let layout;

    beforeEach(() => {
        document.body.innerHTML = '<div id="gl-container"></div>';
        containerEl = document.getElementById('gl-container');
        localStorage.clear();
        layout = new GridLayoutManager(containerEl);
    });

    it('initializes the CSS Grid DOM structure when empty', () => {
        assert.ok(containerEl.classList.contains('ide-layout'));
        assert.ok(containerEl.querySelector('#pane-left'));
        assert.ok(containerEl.querySelector('#splitter-left'));
        assert.ok(containerEl.querySelector('#pane-center'));
        assert.ok(containerEl.querySelector('#pane-main'));
        assert.ok(containerEl.querySelector('#splitter-bottom'));
        assert.ok(containerEl.querySelector('#pane-bottom'));
        assert.ok(containerEl.querySelector('#splitter-right'));
        assert.ok(containerEl.querySelector('#pane-right'));
    });

    it('registers and mounts components into their designated panes', () => {
        const mounted = {};
        const events = {};

        const factory = (name) => (container) => {
            mounted[name] = container;
            events[name] = [];
            container.on('show', () => events[name].push('show'));
            container.on('hide', () => events[name].push('hide'));
            container.on('resize', () => events[name].push('resize'));
            const div = document.createElement('div');
            div.textContent = name;
            container.element.appendChild(div);
        };

        layout.registerComponentFactoryFunction('DisplayControls', factory('DisplayControls'));
        layout.registerComponentFactoryFunction('LayoutViewer', factory('LayoutViewer'));
        layout.registerComponentFactoryFunction('SchematicWidget', factory('SchematicWidget'));
        layout.registerComponentFactoryFunction('TclConsole', factory('TclConsole'));
        layout.registerComponentFactoryFunction('Inspector', factory('Inspector'));
        layout.registerComponentFactoryFunction('TimingWidget', factory('TimingWidget'));

        layout.loadLayout({});

        // Check DOM placement
        const leftContent = containerEl.querySelector('#pane-left-content');
        assert.ok(leftContent.querySelector('#cmp-DisplayControls'));

        const mainContent = containerEl.querySelector('#pane-main-content');
        assert.ok(mainContent.querySelector('#cmp-LayoutViewer'));
        assert.ok(mainContent.querySelector('#cmp-SchematicWidget'));

        const bottomContent = containerEl.querySelector('#pane-bottom-content');
        assert.ok(bottomContent.querySelector('#cmp-TclConsole'));

        const rightContent = containerEl.querySelector('#pane-right-content');
        assert.ok(rightContent.querySelector('#cmp-Inspector'));
        assert.ok(rightContent.querySelector('#cmp-TimingWidget'));

        // Default tabs should be active
        assert.equal(layout.activeTabs.left, 'DisplayControls');
        assert.equal(layout.activeTabs.main, 'LayoutViewer');
        assert.equal(layout.activeTabs.bottom, 'TclConsole');
        assert.equal(layout.activeTabs.right, 'Inspector');

        // Active component containers have .active
        assert.ok(containerEl.querySelector('#cmp-LayoutViewer').classList.contains('active'));
        assert.ok(!containerEl.querySelector('#cmp-SchematicWidget').classList.contains('active'));
    });

    it('switches tabs and delivers lifecycle events', () => {
        const events = { LayoutViewer: [], SchematicWidget: [] };

        layout.registerComponentFactoryFunction('LayoutViewer', (c) => {
            c.on('show', () => events.LayoutViewer.push('show'));
            c.on('hide', () => events.LayoutViewer.push('hide'));
        });
        layout.registerComponentFactoryFunction('SchematicWidget', (c) => {
            c.on('show', () => events.SchematicWidget.push('show'));
            c.on('hide', () => events.SchematicWidget.push('hide'));
        });

        layout.loadLayout({});

        // Initial state
        assert.equal(layout.activeTabs.main, 'LayoutViewer');

        // Switch to Schematic
        layout.activateTab('SchematicWidget');

        assert.equal(layout.activeTabs.main, 'SchematicWidget');
        assert.ok(events.LayoutViewer.includes('hide'));
        assert.ok(events.SchematicWidget.includes('show'));
        assert.ok(containerEl.querySelector('#cmp-SchematicWidget').classList.contains('active'));
        assert.ok(!containerEl.querySelector('#cmp-LayoutViewer').classList.contains('active'));
        assert.ok(layout.isComponentActive(layout.components.get('SchematicWidget')));
        assert.ok(!layout.isComponentActive(layout.components.get('LayoutViewer')));
    });

    it('closes a closable tab and switches to another tab in the pane', () => {
        layout.registerComponentFactoryFunction('Inspector', () => {});
        layout.registerComponentFactoryFunction('TimingWidget', () => {});

        layout.loadLayout({});
        assert.equal(layout.activeTabs.right, 'Inspector');

        layout.closeComponent('Inspector');

        assert.equal(layout.activeTabs.right, 'TimingWidget');
        assert.ok(layout.closedComponents.has('Inspector'));
        const inspTab = containerEl.querySelector('#pane-right-tabs [data-component="Inspector"]');
        assert.equal(inspTab.style.display, 'none');
    });

    it('focusComponent reopens a closed component and activates it', () => {
        layout.registerComponentFactoryFunction('Inspector', () => {});
        layout.registerComponentFactoryFunction('TimingWidget', () => {});

        layout.loadLayout({});
        layout.closeComponent('Inspector');
        assert.equal(layout.activeTabs.right, 'TimingWidget');

        layout.focusComponent('Inspector');
        assert.equal(layout.activeTabs.right, 'Inspector');
        assert.ok(!layout.closedComponents.has('Inspector'));
        const inspTab = containerEl.querySelector('#pane-right-tabs [data-component="Inspector"]');
        assert.equal(inspTab.style.display, '');
    });

    it('collapses and restores panes via controls, gutter buttons, and classes', () => {
        layout.registerComponentFactoryFunction('DisplayControls', () => {});
        layout.registerComponentFactoryFunction('Inspector', () => {});
        layout.registerComponentFactoryFunction('TclConsole', () => {});
        layout.loadLayout({});

        // Left pane: collapse via helper / API, restore via gutter expand button
        const gutterLeft = containerEl.querySelector('#gutter-expand-left');
        assert.ok(gutterLeft);
        layout.collapsePane('left');
        assert.ok(containerEl.classList.contains('hide-left'));
        gutterLeft.click();
        assert.ok(!containerEl.classList.contains('hide-left'));

        // Right pane: collapse via header button, restore via gutter button
        const btnCollapseRight = containerEl.querySelector('#btn-collapse-right');
        const gutterRight = containerEl.querySelector('#gutter-expand-right');
        assert.ok(btnCollapseRight);
        assert.ok(gutterRight);
        btnCollapseRight.click();
        assert.ok(containerEl.classList.contains('hide-right'));
        gutterRight.click();
        assert.ok(!containerEl.classList.contains('hide-right'));

        // Bottom pane: collapse via header button, restore via gutter button
        const btnCollapseBottom = containerEl.querySelector('#btn-collapse-bottom');
        const gutterBottom = containerEl.querySelector('#gutter-expand-bottom');
        assert.ok(btnCollapseBottom);
        assert.ok(gutterBottom);
        btnCollapseBottom.click();
        assert.ok(containerEl.querySelector('#pane-center').classList.contains('hide-bottom'));
        gutterBottom.click();
        assert.ok(!containerEl.querySelector('#pane-center').classList.contains('hide-bottom'));

        // Focusing component in collapsed pane automatically unhides it
        containerEl.classList.add('hide-left');
        layout.focusComponent('DisplayControls');
        assert.ok(!containerEl.classList.contains('hide-left'));
    });

    it('resets layout to defaults', () => {
        layout.registerComponentFactoryFunction('DisplayControls', () => {});
        layout.registerComponentFactoryFunction('LayoutViewer', () => {});
        layout.registerComponentFactoryFunction('SchematicWidget', () => {});
        layout.registerComponentFactoryFunction('TclConsole', () => {});
        layout.registerComponentFactoryFunction('Inspector', () => {});
        layout.loadLayout({});

        // Alter dimensions, collapse panes, maximize main
        containerEl.style.setProperty('--pane-left-width', '500px');
        containerEl.style.setProperty('--pane-right-width', '500px');
        containerEl.style.setProperty('--pane-bottom-height', '400px');
        layout.collapsePane('left');
        layout.collapsePane('right');
        layout.collapsePane('bottom');
        containerEl.querySelector('#pane-main').classList.add('maximized');

        // Reset
        layout.resetLayout();

        assert.equal(containerEl.style.getPropertyValue('--pane-left-width'), '240px');
        assert.equal(containerEl.style.getPropertyValue('--pane-right-width'), '340px');
        assert.equal(containerEl.style.getPropertyValue('--pane-bottom-height'), '220px');
        assert.ok(!containerEl.classList.contains('hide-left'));
        assert.ok(!containerEl.classList.contains('hide-right'));
        assert.ok(!containerEl.querySelector('#pane-center').classList.contains('hide-bottom'));
        assert.ok(!containerEl.querySelector('#pane-main').classList.contains('maximized'));
        assert.equal(layout.activeTabs.main, 'LayoutViewer');
        assert.equal(layout.activeTabs.left, 'DisplayControls');
    });

    it('renders empty-state when all main views are closed and allows reset', () => {
        layout.registerComponentFactoryFunction('LayoutViewer', () => {});
        layout.registerComponentFactoryFunction('DisplayControls', () => {});
        layout.loadLayout({});

        const emptyState = containerEl.querySelector('#ide-empty-state');
        assert.ok(emptyState);
        assert.equal(emptyState.style.display, 'none');

        // Simulate no active tab in main
        layout.activeTabs.main = null;
        layout._updateEmptyState();
        assert.equal(emptyState.style.display, 'flex');

        // Clicking reset restores layout and hides empty state
        const resetBtn = emptyState.querySelector('#btn-empty-reset-layout');
        assert.ok(resetBtn);
        resetBtn.click();
        assert.equal(emptyState.style.display, 'none');
        assert.equal(layout.activeTabs.main, 'LayoutViewer');
    });

    it('maximizes and restores main pane', () => {
        layout.registerComponentFactoryFunction('LayoutViewer', () => {});
        layout.loadLayout({});

        const btnMaxMain = containerEl.querySelector('#btn-maximize-main');
        assert.ok(btnMaxMain);

        btnMaxMain.click();
        const mainPane = containerEl.querySelector('#pane-main');
        assert.ok(mainPane.classList.contains('maximized'));
        assert.ok(containerEl.classList.contains('maximized-main'));

        btnMaxMain.click();
        assert.ok(!mainPane.classList.contains('maximized'));
        assert.ok(!containerEl.classList.contains('maximized-main'));
    });

    it('toggles maximized pane state and restores layout correctly', () => {
        const bottomPane = containerEl.querySelector('#pane-bottom');
        const btnMaxBottom = containerEl.querySelector('#btn-maximize-bottom');
        const btnColBottom = containerEl.querySelector('#btn-collapse-bottom');

        assert.ok(btnMaxBottom);
        assert.ok(btnColBottom);

        // Maximize bottom pane
        btnMaxBottom.click();
        assert.ok(layout.isPaneMaximized('bottom'));
        assert.ok(bottomPane.classList.contains('maximized'));

        // Restore from maximize
        btnMaxBottom.click();
        assert.ok(!layout.isPaneMaximized('bottom'));
        assert.ok(!bottomPane.classList.contains('maximized'));

        // Maximize via API
        layout.maximizePane('bottom');
        assert.ok(layout.isPaneMaximized('bottom'));
        assert.ok(bottomPane.classList.contains('maximized'));

        // Restore via restoreMaximized API
        layout.restoreMaximized();
        assert.ok(!layout.isPaneMaximized('bottom'));
        assert.ok(!bottomPane.classList.contains('maximized'));

        // Normal collapsing works
        layout.collapsePane('bottom');
        assert.ok(layout.isPaneCollapsed('bottom'));
        layout.restorePane('bottom');
        assert.ok(!layout.isPaneCollapsed('bottom'));
    });

    it('persists and restores layout dimensions and active tabs', () => {
        layout.registerComponentFactoryFunction('DisplayControls', () => {});
        layout.registerComponentFactoryFunction('LayoutViewer', () => {});
        layout.registerComponentFactoryFunction('SchematicWidget', () => {});
        layout.registerComponentFactoryFunction('TclConsole', () => {});
        layout.registerComponentFactoryFunction('Inspector', () => {});

        layout.loadLayout({});

        // Set custom dimensions
        containerEl.style.setProperty('--pane-left-width', '280px');
        containerEl.style.setProperty('--pane-right-width', '380px');
        containerEl.style.setProperty('--pane-bottom-height', '260px');
        layout.activateTab('SchematicWidget');

        layout.saveLayout();

        // Create a new layout instance that should restore saved state
        const layout2 = new GridLayoutManager(containerEl);
        layout2.registerComponentFactoryFunction('DisplayControls', () => {});
        layout2.registerComponentFactoryFunction('LayoutViewer', () => {});
        layout2.registerComponentFactoryFunction('SchematicWidget', () => {});
        layout2.registerComponentFactoryFunction('TclConsole', () => {});
        layout2.registerComponentFactoryFunction('Inspector', () => {});

        layout2.loadLayout({});

        assert.equal(containerEl.style.getPropertyValue('--pane-left-width'), '280px');
        assert.equal(containerEl.style.getPropertyValue('--pane-right-width'), '380px');
        assert.equal(containerEl.style.getPropertyValue('--pane-bottom-height'), '260px');
        assert.equal(layout2.activeTabs.main, 'SchematicWidget');
    });

    it('renders and manages <ide-pane> custom elements properly', () => {
        const pane = document.createElement('ide-pane');
        pane.id = 'pane-custom';
        pane.setAttribute('name', 'custom');
        pane.setAttribute('maximizable', '');
        pane.setAttribute('collapsible', '');

        document.body.appendChild(pane);

        assert.equal(pane.tagName.toLowerCase(), 'ide-pane');
        assert.ok(pane.classList.contains('ide-pane'));
        assert.ok(pane.classList.contains('ide-pane-custom'));

        assert.ok(pane.headerEl);
        assert.ok(pane.tabsEl);
        assert.ok(pane.controlsEl);
        assert.ok(pane.contentEl);
        assert.ok(pane.maximizeBtn);
        assert.ok(pane.collapseBtn);

        let maxEventFired = false;
        let colEventFired = false;
        pane.addEventListener('ide-pane-maximize', (e) => {
            maxEventFired = true;
            assert.equal(e.detail.pane, 'custom');
        });
        pane.addEventListener('ide-pane-collapse', (e) => {
            colEventFired = true;
            assert.equal(e.detail.pane, 'custom');
        });

        pane.maximizeBtn.click();
        assert.ok(maxEventFired);

        pane.collapseBtn.click();
        assert.ok(colEventFired);

        pane.setMaximized(true);
        assert.ok(pane.classList.contains('maximized'));
        assert.equal(pane.maximizeBtn.textContent, '⧉');

        pane.setMaximized(false);
        assert.ok(!pane.classList.contains('maximized'));
        assert.equal(pane.maximizeBtn.textContent, '⤢');

        pane.remove();
    });

    it('renders and manages <ide-splitter> custom elements properly', () => {
        const splitter = document.createElement('ide-splitter');
        splitter.id = 'splitter-test';
        splitter.setAttribute('for', 'left');
        splitter.setAttribute('orientation', 'vertical');

        document.body.appendChild(splitter);

        assert.equal(splitter.tagName.toLowerCase(), 'ide-splitter');
        assert.equal(splitter.getAttribute('role'), 'separator');
        assert.equal(splitter.getAttribute('aria-orientation'), 'vertical');
        assert.ok(splitter.classList.contains('ide-splitter'));
        assert.ok(splitter.classList.contains('ide-splitter-v'));
        assert.equal(splitter.targetPane, 'left');
        assert.equal(splitter.isVertical, true);

        let resetFired = false;
        splitter.addEventListener('ide-splitter-reset', (e) => {
            resetFired = true;
            assert.equal(e.detail.for, 'left');
        });

        splitter.dispatchEvent(new Event('dblclick'));
        assert.ok(resetFired);

        // Test dragging with window event listeners
        let dragDetail = null;
        splitter.addEventListener('ide-splitter-drag', (e) => {
            dragDetail = e.detail;
        });

        const PointerEvt = window.PointerEvent || window.MouseEvent;
        splitter.dispatchEvent(new PointerEvt('pointerdown', { bubbles: true, clientX: 100, clientY: 50 }));
        assert.ok(splitter.classList.contains('dragging'));
        assert.ok(document.body.classList.contains('is-resizing'));

        window.dispatchEvent(new PointerEvt('pointermove', { bubbles: true, clientX: 140, clientY: 50 }));
        assert.ok(dragDetail);
        assert.equal(dragDetail.delta, 40);

        window.dispatchEvent(new PointerEvt('pointerup', { bubbles: true, clientX: 140, clientY: 50 }));
        assert.ok(!splitter.classList.contains('dragging'));
        assert.ok(!document.body.classList.contains('is-resizing'));

        splitter.remove();
    });

    it('creates and manages header, tabs, controls, and content in <ide-pane>', () => {
        const pane = document.createElement('ide-pane');
        pane.id = 'pane-structure';
        pane.setAttribute('name', 'structure');
        pane.setAttribute('maximizable', '');
        pane.setAttribute('collapsible', '');

        document.body.appendChild(pane);

        assert.ok(pane.querySelector('.ide-header'));
        assert.ok(pane.querySelector('.ide-tabs'));
        assert.ok(pane.querySelector('.ide-controls'));
        assert.ok(pane.querySelector('.ide-content'));
        assert.ok(pane.querySelector('#btn-maximize-structure'));
        assert.ok(pane.querySelector('#btn-collapse-structure'));

        pane.remove();
    });

    it('renders and manages <ide-tab> custom elements properly', () => {
        const tab = document.createElement('ide-tab');
        tab.id = 'tab-test-component';
        tab.setAttribute('for', 'test-component');
        tab.setAttribute('title', 'Test Component');
        tab.setAttribute('closable', '');

        document.body.appendChild(tab);

        assert.equal(tab.tagName.toLowerCase(), 'ide-tab');
        assert.ok(tab.classList.contains('ide-tab'));
        assert.ok(tab.classList.contains('lm_tab'));
        assert.equal(tab.getAttribute('role'), 'tab');
        assert.equal(tab.targetComponent, 'test-component');
        assert.equal(tab.titleText, 'Test Component');
        assert.ok(tab.titleEl);
        assert.ok(tab.closeBtn);

        let selectFired = false;
        let closeFired = false;

        tab.addEventListener('tab-select', (e) => {
            selectFired = true;
            assert.equal(e.detail.componentType, 'test-component');
        });

        tab.addEventListener('tab-close', (e) => {
            closeFired = true;
            assert.equal(e.detail.componentType, 'test-component');
        });

        tab.click();
        assert.ok(selectFired);

        tab.closeBtn.click();
        assert.ok(closeFired);

        tab.setActive(true);
        assert.ok(tab.classList.contains('active'));
        assert.ok(tab.classList.contains('lm_active'));
        assert.equal(tab.getAttribute('aria-selected'), 'true');

        tab.setActive(false);
        assert.ok(!tab.classList.contains('active'));
        assert.equal(tab.getAttribute('aria-selected'), 'false');

        tab.remove();
    });
});
