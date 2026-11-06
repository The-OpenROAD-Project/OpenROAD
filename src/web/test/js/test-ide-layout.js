// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it, beforeEach } from 'node:test';
import assert from 'node:assert/strict';
import {
    GridLayoutManager, IdePaneElement, IdeSplitterElement, IdeTabElement,
    TabIcon, COMPONENT_CONFIG
} from '../../src/grid-layout.js';

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
        const leftPane = containerEl.querySelector('#pane-left');
        assert.ok(leftPane.querySelector('#cmp-DisplayControls'));

        const mainPane = containerEl.querySelector('#pane-main');
        assert.ok(mainPane.querySelector('#cmp-LayoutViewer'));
        assert.ok(mainPane.querySelector('#cmp-SchematicWidget'));

        const bottomPane = containerEl.querySelector('#pane-bottom');
        assert.ok(bottomPane.querySelector('#cmp-TclConsole'));

        const rightPane = containerEl.querySelector('#pane-right');
        assert.ok(rightPane.querySelector('#cmp-Inspector'));
        assert.ok(rightPane.querySelector('#cmp-TimingWidget'));

        // Default tabs should be active
        assert.equal(layout.activeTabs.left, 'DisplayControls');
        assert.equal(layout.activeTabs.main, 'LayoutViewer');
        assert.equal(layout.activeTabs.bottom, 'TclConsole');
        assert.equal(layout.activeTabs.right, 'Inspector');

        // Active component containers have .active
        assert.ok(containerEl.querySelector('#cmp-LayoutViewer').classList.contains('active'));
        assert.ok(!containerEl.querySelector('#cmp-SchematicWidget').classList.contains('active'));

        // Tabs receive icons from COMPONENT_CONFIG
        const layoutTab = containerEl.querySelector('#tab-LayoutViewer');
        assert.equal(layoutTab.icon, TabIcon.LAYOUT);
        const tclTab = containerEl.querySelector('#tab-TclConsole');
        assert.equal(tclTab.icon, TabIcon.TCL_CONSOLE);
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
        const inspTab = containerEl.querySelector('#pane-right [data-component="Inspector"]');
        assert.ok(inspTab.hidden);
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
        const inspTab = containerEl.querySelector('#pane-right [data-component="Inspector"]');
        assert.ok(!inspTab.hidden);
    });

    it('collapses and restores panes via controls, gutter buttons, and classes', async () => {
        layout.registerComponentFactoryFunction('DisplayControls', () => {});
        layout.registerComponentFactoryFunction('Inspector', () => {});
        layout.registerComponentFactoryFunction('TclConsole', () => {});
        layout.loadLayout({});

        await Promise.all([
            layout.panes.right.updateComplete,
            layout.panes.bottom.updateComplete,
        ]);

        // Left pane: collapse via helper / API, restore via gutter expand button
        const gutterLeft = containerEl.querySelector('#gutter-expand-left');
        assert.ok(gutterLeft);
        layout.collapsePane('left');
        assert.ok(containerEl.classList.contains('hide-left'));
        gutterLeft.click();
        assert.ok(!containerEl.classList.contains('hide-left'));

        // Right pane: collapse via header button, restore via gutter button
        const btnCollapseRight = layout.panes.right.shadowRoot.querySelector('.ide-btn-collapse');
        const gutterRight = containerEl.querySelector('#gutter-expand-right');
        assert.ok(btnCollapseRight);
        assert.ok(gutterRight);
        btnCollapseRight.click();
        assert.ok(containerEl.classList.contains('hide-right'));
        gutterRight.click();
        assert.ok(!containerEl.classList.contains('hide-right'));

        // Bottom pane: collapse via header button, restore via gutter button
        const btnCollapseBottom = layout.panes.bottom.shadowRoot.querySelector('.ide-btn-collapse');
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

    it('resets layout to defaults', async () => {
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
        layout.maximizePane('main');
        await layout.panes.main.updateComplete;
        assert.ok(layout.panes.main.hasAttribute('maximized'));

        // Reset
        layout.resetLayout();
        await layout.panes.main.updateComplete;

        assert.equal(containerEl.style.getPropertyValue('--pane-left-width'), '240px');
        assert.equal(containerEl.style.getPropertyValue('--pane-right-width'), '340px');
        assert.equal(containerEl.style.getPropertyValue('--pane-bottom-height'), '220px');
        assert.ok(!containerEl.classList.contains('hide-left'));
        assert.ok(!containerEl.classList.contains('hide-right'));
        assert.ok(!containerEl.querySelector('#pane-center').classList.contains('hide-bottom'));
        assert.ok(!layout.panes.main.hasAttribute('maximized'));
        assert.equal(layout.activeTabs.main, 'LayoutViewer');
        assert.equal(layout.activeTabs.left, 'DisplayControls');
    });

    it('maximizes and restores main pane', async () => {
        layout.registerComponentFactoryFunction('LayoutViewer', () => {});
        layout.loadLayout({});

        const mainPane = layout.panes.main;
        await mainPane.updateComplete;
        const btnMaxMain = mainPane.shadowRoot.querySelector('.ide-btn-maximize');
        assert.ok(btnMaxMain);

        btnMaxMain.click();
        await mainPane.updateComplete;
        assert.ok(mainPane.maximized);
        assert.ok(mainPane.hasAttribute('maximized'));
        assert.ok(containerEl.classList.contains('maximized-main'));

        btnMaxMain.click();
        await mainPane.updateComplete;
        assert.ok(!mainPane.maximized);
        assert.ok(!mainPane.hasAttribute('maximized'));
        assert.ok(!containerEl.classList.contains('maximized-main'));
    });

    it('toggles maximized pane state and restores layout correctly', async () => {
        const bottomPane = layout.panes.bottom;
        await bottomPane.updateComplete;
        const btnMaxBottom = bottomPane.shadowRoot.querySelector('.ide-btn-maximize');
        const btnColBottom = bottomPane.shadowRoot.querySelector('.ide-btn-collapse');

        assert.ok(btnMaxBottom);
        assert.ok(btnColBottom);

        // Maximize bottom pane
        btnMaxBottom.click();
        await bottomPane.updateComplete;
        assert.ok(layout.isPaneMaximized('bottom'));
        assert.ok(bottomPane.maximized);
        assert.ok(bottomPane.hasAttribute('maximized'));

        // Restore from maximize
        btnMaxBottom.click();
        await bottomPane.updateComplete;
        assert.ok(!layout.isPaneMaximized('bottom'));
        assert.ok(!bottomPane.maximized);
        assert.ok(!bottomPane.hasAttribute('maximized'));

        // Maximize via API
        layout.maximizePane('bottom');
        await bottomPane.updateComplete;
        assert.ok(layout.isPaneMaximized('bottom'));
        assert.ok(bottomPane.maximized);
        assert.ok(bottomPane.hasAttribute('maximized'));

        // Restore via restoreMaximized API
        layout.restoreMaximized();
        await bottomPane.updateComplete;
        assert.ok(!layout.isPaneMaximized('bottom'));
        assert.ok(!bottomPane.maximized);
        assert.ok(!bottomPane.hasAttribute('maximized'));

        // Normal collapsing works
        layout.collapsePane('bottom');
        await bottomPane.updateComplete;
        assert.ok(layout.isPaneCollapsed('bottom'));
        assert.ok(bottomPane.collapsed);
        layout.restorePane('bottom');
        await bottomPane.updateComplete;
        assert.ok(!layout.isPaneCollapsed('bottom'));
        assert.ok(!bottomPane.collapsed);
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

    it('renders and manages <ide-pane> custom elements properly', async () => {
        const pane = document.createElement('ide-pane');
        pane.id = 'pane-custom';
        pane.setAttribute('name', 'custom');
        pane.setAttribute('maximizable', '');
        pane.setAttribute('collapsible', '');

        document.body.appendChild(pane);
        await pane.updateComplete;

        assert.equal(pane.tagName.toLowerCase(), 'ide-pane');
        assert.ok(pane.classList.contains('ide-pane'));
        assert.ok(pane.classList.contains('ide-pane-custom'));

        const maxBtn = pane.shadowRoot.querySelector('.ide-btn-maximize');
        const colBtn = pane.shadowRoot.querySelector('.ide-btn-collapse');
        assert.ok(maxBtn);
        assert.ok(colBtn);

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

        maxBtn.click();
        assert.ok(maxEventFired);

        colBtn.click();
        assert.ok(colEventFired);

        pane.setMaximized(true);
        await pane.updateComplete;
        assert.ok(pane.hasAttribute('maximized'));
        assert.ok(pane.maximized);
        assert.equal(maxBtn.textContent, '⧉');

        pane.setMaximized(false);
        await pane.updateComplete;
        assert.ok(!pane.hasAttribute('maximized'));
        assert.ok(!pane.maximized);
        assert.equal(maxBtn.textContent, '⤢');

        pane.remove();
    });

    it('renders and manages <ide-splitter> custom elements properly', async () => {
        const splitter = document.createElement('ide-splitter');
        splitter.id = 'splitter-test';
        splitter.setAttribute('for', 'left');
        splitter.setAttribute('orientation', 'vertical');

        document.body.appendChild(splitter);
        await splitter.updateComplete;

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

    it('creates and manages header, tabs, controls, and content in <ide-pane>', async () => {
        const pane = document.createElement('ide-pane');
        pane.id = 'pane-structure';
        pane.setAttribute('name', 'structure');
        pane.setAttribute('maximizable', '');
        pane.setAttribute('collapsible', '');

        document.body.appendChild(pane);
        await pane.updateComplete;

        assert.ok(pane.shadowRoot.querySelector('.ide-header'));
        assert.ok(pane.shadowRoot.querySelector('.ide-tabs'));
        assert.ok(pane.shadowRoot.querySelector('.ide-controls'));
        assert.ok(pane.shadowRoot.querySelector('.ide-content'));
        assert.ok(pane.shadowRoot.querySelector('#btn-maximize-structure'));
        assert.ok(pane.shadowRoot.querySelector('#btn-collapse-structure'));

        pane.remove();
    });

    it('renders and manages <ide-tab> custom elements properly', async () => {
        const tab = document.createElement('ide-tab');
        tab.id = 'tab-test-component';
        tab.setAttribute('for', 'test-component');
        tab.setAttribute('title', 'Test Component');
        tab.setAttribute('closable', '');

        document.body.appendChild(tab);
        await tab.updateComplete;

        assert.equal(tab.tagName.toLowerCase(), 'ide-tab');
        assert.ok(tab.shadowRoot, 'Expected shadowRoot to be created');
        assert.ok(tab.classList.contains('ide-tab'));
        assert.equal(tab.getAttribute('role'), 'tab');
        assert.equal(tab.targetComponent, 'test-component');
        assert.equal(tab.title, 'Test Component');
        assert.ok(tab.shadowRoot.querySelector('.ide-tab-title'));
        assert.ok(tab.shadowRoot.querySelector('.ide-tab-close'));

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

        tab.shadowRoot.querySelector('.ide-tab-close').click();
        assert.ok(closeFired);

        tab.setActive(true);
        await tab.updateComplete;
        assert.ok(tab.hasAttribute('active'));
        assert.ok(tab.active);
        assert.equal(tab.getAttribute('aria-selected'), 'true');

        tab.setActive(false);
        await tab.updateComplete;
        assert.ok(!tab.hasAttribute('active'));
        assert.ok(!tab.active);
        assert.equal(tab.getAttribute('aria-selected'), 'false');

        // Test Lit reactivity with Shadow DOM
        tab.title = 'Updated Title';
        tab.active = true;
        tab.closable = false;
        await tab.updateComplete;

        assert.equal(tab.shadowRoot.querySelector('.ide-tab-title').textContent, 'Updated Title');
        assert.ok(tab.hasAttribute('active'));
        assert.ok(tab.active);
        assert.equal(tab.getAttribute('aria-selected'), 'true');
        assert.equal(tab.shadowRoot.querySelector('.ide-tab-close'), null);

        // Test custom DOM element icon property support
        const testIcon = document.createElement('span');
        testIcon.className = 'custom-test-icon';
        tab.icon = testIcon;
        await tab.updateComplete;
        assert.ok(tab.shadowRoot.querySelector('.custom-test-icon'));

        // Test TabIcon enum rendering
        tab.icon = TabIcon.LAYOUT;
        await tab.updateComplete;
        assert.ok(tab.shadowRoot.querySelector('.or-tab-icon svg'));

        // Test updating to another TabIcon
        tab.icon = TabIcon.TCL_CONSOLE;
        await tab.updateComplete;
        assert.ok(tab.shadowRoot.querySelector('.or-tab-icon svg'));

        // Tabs without an icon do not render an icon container
        tab.icon = null;
        await tab.updateComplete;
        assert.equal(tab.shadowRoot.querySelector('.or-tab-icon'), null);

        tab.remove();
    });

    it('defines TabIcon enum with all-caps keys and binds them in COMPONENT_CONFIG', () => {
        const expectedKeys = [
            'LAYOUT', 'SCHEMATIC', 'VIEWER_3D', 'DISPLAY_CONTROLS', 'INSPECTOR',
            'HIERARCHY', 'TIMING', 'DRC', 'SELECT_HIGHLIGHT', 'CLOCK_TREE',
            'CHARTS', 'TCL_CONSOLE', 'HELP'
        ];
        for (const key of expectedKeys) {
            assert.ok(key in TabIcon, `Expected ${key} in TabIcon`);
            assert.equal(key, key.toUpperCase(), `Key ${key} must be all-caps`);
            assert.ok(TabIcon[key], `Expected TabIcon.${key} to be defined`);
        }

        // Verify COMPONENT_CONFIG has icons configured
        assert.equal(COMPONENT_CONFIG.LayoutViewer.icon, TabIcon.LAYOUT);
        assert.equal(COMPONENT_CONFIG.SchematicWidget.icon, TabIcon.SCHEMATIC);
        assert.equal(COMPONENT_CONFIG['3DViewer'].icon, TabIcon.VIEWER_3D);
        assert.equal(COMPONENT_CONFIG.DisplayControls.icon, TabIcon.DISPLAY_CONTROLS);
        assert.equal(COMPONENT_CONFIG.Inspector.icon, TabIcon.INSPECTOR);
        assert.equal(COMPONENT_CONFIG.Browser.icon, TabIcon.HIERARCHY);
        assert.equal(COMPONENT_CONFIG.TimingWidget.icon, TabIcon.TIMING);
        assert.equal(COMPONENT_CONFIG.DRCWidget.icon, TabIcon.DRC);
        assert.equal(COMPONENT_CONFIG.SelectHighlight.icon, TabIcon.SELECT_HIGHLIGHT);
        assert.equal(COMPONENT_CONFIG.ClockWidget.icon, TabIcon.CLOCK_TREE);
        assert.equal(COMPONENT_CONFIG.ChartsWidget.icon, TabIcon.CHARTS);
        assert.equal(COMPONENT_CONFIG.TclConsole.icon, TabIcon.TCL_CONSOLE);
        assert.equal(COMPONENT_CONFIG.HelpWidget.icon, TabIcon.HELP);
    });
});
