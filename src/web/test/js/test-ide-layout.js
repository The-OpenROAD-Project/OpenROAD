// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import './setup-dom.js';
import { describe, it, beforeEach } from 'node:test';
import assert from 'node:assert/strict';
import {
    GridLayoutManager, IdePaneElement, IdeSplitterElement, IdeTabElement,
    TabIcon, COMPONENT_CONFIG
} from '../../src/grid-layout.js';

const PointerEvt = window.PointerEvent || window.MouseEvent;

// jsdom has no layout: give a tab strip the geometry a browser would.
function stubTabStrip(tabsEl, { scrollWidth, clientWidth }) {
    let scrollLeft = 0;
    Object.defineProperty(tabsEl, 'scrollWidth', { value: scrollWidth, configurable: true });
    Object.defineProperty(tabsEl, 'clientWidth', { value: clientWidth, configurable: true });
    Object.defineProperty(tabsEl, 'scrollLeft', {
        get: () => scrollLeft,
        set: (v) => { scrollLeft = v; },
        configurable: true,
    });
    tabsEl.dispatchEvent(new Event('scroll'));
}

// A keydown that crosses into the pane's shadow DOM; returns the event.
function pressKey(target, key) {
    const e = new window.KeyboardEvent('keydown', {
        key, bubbles: true, composed: true, cancelable: true,
    });
    target.dispatchEvent(e);
    return e;
}

// Lets MutationObserver and slotchange callbacks run.
const tick = () => new Promise(resolve => setTimeout(resolve, 0));

const menuOf = (pane) => pane.shadowRoot.querySelector('.ide-tab-menu');
const menuItems = (pane) => [...pane.shadowRoot.querySelectorAll('.ide-tab-menu-item')];
const focusedTitle = (pane) => pane.shadowRoot.activeElement
    ?.querySelector('.ide-tab-menu-title')?.textContent;

describe('GridLayoutManager', () => {
    let containerEl;
    let layout;

    beforeEach(() => {
        document.body.innerHTML = '<div id="gl-container"></div>';
        containerEl = document.getElementById('gl-container');
        localStorage.clear();
        layout = new GridLayoutManager(containerEl);
    });

    async function mountRightPane() {
        for (const type of ['Inspector', 'Browser', 'TimingWidget', 'ChartsWidget']) {
            layout.registerComponentFactoryFunction(type, () => {});
        }
        layout.loadLayout({});
        const pane = layout.panes.right;
        await pane.updateComplete;
        return { pane, tabsEl: pane.shadowRoot.querySelector('.ide-tabs') };
    }

    // The right pane with its tabs overflowing; with `open`, the ▾ menu too.
    async function mountOverflowingPane({ open = false } = {}) {
        const { pane, tabsEl } = await mountRightPane();
        stubTabStrip(tabsEl, { scrollWidth: 700, clientWidth: 300 });
        await pane.updateComplete;
        const btn = pane.shadowRoot.querySelector('.ide-btn-overflow');
        if (open) {
            btn.click();
            await pane.updateComplete;
        }
        return { pane, tabsEl, btn };
    }

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

    it('focusComponent with reveal restores a maximized pane that hides the target', () => {
        layout.registerComponentFactoryFunction('LayoutViewer', () => {});
        layout.registerComponentFactoryFunction('TimingWidget', () => {});
        layout.registerComponentFactoryFunction('ChartsWidget', () => {});
        layout.loadLayout({});

        // Programmatic focus leaves the user's maximized pane alone.
        layout.maximizePane('main');
        layout.focusComponent('ChartsWidget');
        assert.ok(containerEl.classList.contains('maximized-main'));

        layout.focusComponent('ChartsWidget', { reveal: true });
        assert.ok(!layout.isPaneMaximized());
        assert.ok(!containerEl.classList.contains('maximized-main'));
        assert.equal(layout.activeTabs.right, 'ChartsWidget');

        // The target is in the maximized pane itself: nothing to restore.
        layout.maximizePane('right');
        layout.focusComponent('TimingWidget', { reveal: true });
        assert.ok(containerEl.classList.contains('maximized-right'));
        assert.equal(layout.activeTabs.right, 'TimingWidget');

        layout.focusComponent('ChartsWidget', null);
        assert.equal(layout.activeTabs.right, 'ChartsWidget');
    });

    it('shows the overflow button and edge fades only while the tabs overflow', async () => {
        const { pane, tabsEl } = await mountRightPane();
        const btn = pane.shadowRoot.querySelector('.ide-btn-overflow');
        assert.ok(btn.classList.contains('idle'), 'laid out, but hidden until the tabs overflow');
        assert.ok(!tabsEl.classList.contains('fade-end'));

        stubTabStrip(tabsEl, { scrollWidth: 700, clientWidth: 300 });
        await pane.updateComplete;
        assert.ok(!btn.classList.contains('idle'));
        assert.ok(!tabsEl.classList.contains('fade-start'));
        assert.ok(tabsEl.classList.contains('fade-end'));

        tabsEl.scrollLeft = 200;
        tabsEl.dispatchEvent(new Event('scroll'));
        await pane.updateComplete;
        assert.ok(tabsEl.classList.contains('fade-start'));
        assert.ok(tabsEl.classList.contains('fade-end'));

        tabsEl.scrollLeft = 400;
        tabsEl.dispatchEvent(new Event('scroll'));
        await pane.updateComplete;
        assert.ok(tabsEl.classList.contains('fade-start'));
        assert.ok(!tabsEl.classList.contains('fade-end'));

        stubTabStrip(tabsEl, { scrollWidth: 300, clientWidth: 300 });
        await pane.updateComplete;
        assert.ok(btn.classList.contains('idle'));
        assert.ok(!tabsEl.classList.contains('fade-start'));
        assert.ok(!tabsEl.classList.contains('fade-end'));
    });

    it('lists the open tabs in the overflow menu and activates the chosen one', async () => {
        const { pane, btn } = await mountOverflowingPane();
        layout.closeComponent('Browser');
        assert.equal(btn.getAttribute('aria-expanded'), 'false');
        btn.click();
        await pane.updateComplete;
        assert.equal(btn.getAttribute('aria-expanded'), 'true');

        const items = menuItems(pane);
        assert.deepEqual(
            items.map(item => item.querySelector('.ide-tab-menu-title').textContent),
            ['Inspector', 'Timing', 'Charts']);
        assert.deepEqual(
            items.map(item => item.getAttribute('aria-checked')),
            ['true', 'false', 'false']);
        assert.ok(items[2].querySelector('.ide-tab-icon svg'));

        // Opening focuses the active tab's item; arrows, Home and End move
        // between items, which stay out of the Tab order.
        assert.ok(items.every(item => item.getAttribute('tabindex') === '-1'));
        assert.equal(pane.shadowRoot.activeElement, items[0]);
        const key = (k) => pressKey(pane.shadowRoot.activeElement, k);
        key('ArrowUp');
        assert.equal(pane.shadowRoot.activeElement, items[2]);
        key('ArrowDown');
        assert.equal(pane.shadowRoot.activeElement, items[0]);
        key('End');
        assert.equal(pane.shadowRoot.activeElement, items[2]);
        key('Home');
        assert.equal(pane.shadowRoot.activeElement, items[0]);
        key('End');

        items[2].click();
        await pane.updateComplete;
        assert.equal(layout.activeTabs.right, 'ChartsWidget');
        assert.equal(menuOf(pane), null);
        assert.equal(btn.getAttribute('aria-expanded'), 'false');
        assert.equal(pane.shadowRoot.activeElement, btn, 'focus returns to the ▾');
    });

    it('lets a click on the overflow button reach document listeners', async () => {
        const { btn } = await mountOverflowingPane();

        // The menu bar and the layer context menu close on a document click.
        let clicks = 0;
        const onClick = () => { clicks++; };
        document.addEventListener('click', onClick);
        try {
            btn.click();
            assert.equal(clicks, 1);
        } finally {
            document.removeEventListener('click', onClick);
        }
    });

    it('keeps keyboard focus and Escape within the overflow menu', async () => {
        const { pane, btn } = await mountOverflowingPane();
        const focused = () => pane.shadowRoot.activeElement;
        const open = async () => {
            btn.click();
            await pane.updateComplete;
            assert.ok(menuOf(pane));
        };

        // Like main.js's shortcuts: registered first, in document capture.
        const seen = [];
        const onKeydown = (e) => seen.push(e.key);
        document.addEventListener('keydown', onKeydown, true);
        try {
            await open();
            assert.ok(!pressKey(document, 'Escape').defaultPrevented);
            await pane.updateComplete;
            assert.ok(menuOf(pane), 'an Escape outside the menu leaves it open');
            assert.equal(seen.length, 1);

            const e = pressKey(focused(), 'Escape');
            await pane.updateComplete;
            assert.ok(e.defaultPrevented);
            assert.equal(menuOf(pane), null);
            assert.equal(focused(), btn);
            assert.equal(seen.length, 1, 'the menu\'s Escape stops at the menu');

            // A letter jumps to the next item starting with it, and is no
            // shortcut ('t' toggles the theme, 'k' the ruler).
            await open();
            assert.ok(pressKey(focused(), 't').defaultPrevented);
            assert.equal(focusedTitle(pane), 'Timing');
            pressKey(focused(), 'h');
            assert.equal(focusedTitle(pane), 'Hierarchy', 'wraps past the end');
            assert.ok(pressKey(focused(), 'x').defaultPrevented, 'no match');
            assert.equal(focusedTitle(pane), 'Hierarchy');
            assert.equal(seen.length, 1);

            // Enter is left to the button.
            assert.ok(!pressKey(focused(), 'Enter').defaultPrevented);
            assert.equal(seen.length, 2);
            pressKey(focused(), 'Escape');
            await pane.updateComplete;
        } finally {
            document.removeEventListener('keydown', onKeydown, true);
        }

        // Focus moving between items keeps it open; leaving the menu closes it.
        await open();
        const focusout = (relatedTarget) => menuItems(pane)[0].dispatchEvent(
            new window.FocusEvent('focusout', { bubbles: true, composed: true, relatedTarget }));
        focusout(menuItems(pane)[1]);
        await pane.updateComplete;
        assert.ok(menuOf(pane));
        focusout(document.body);
        await pane.updateComplete;
        assert.equal(menuOf(pane), null);

        // A press on an item must not move focus off the menu first.
        await open();
        const press = new window.MouseEvent('mousedown', { bubbles: true, cancelable: true });
        menuItems(pane)[1].dispatchEvent(press);
        assert.ok(press.defaultPrevented);

        menuItems(pane)[0].dispatchEvent(
            new PointerEvt('pointerdown', { bubbles: true, composed: true }));
        await pane.updateComplete;
        assert.ok(menuOf(pane), 'a pointerdown inside the menu keeps it open');
        document.body.dispatchEvent(new PointerEvt('pointerdown', { bubbles: true }));
        await pane.updateComplete;
        assert.equal(menuOf(pane), null);

        // Maximizing the pane closes it too.
        await open();
        layout.maximizePane('right');
        await pane.updateComplete;
        assert.equal(menuOf(pane), null);
    });

    it('updates the open overflow menu when the active tab changes', async () => {
        const { pane } = await mountOverflowingPane({ open: true });

        // E.g. a selection response focusing another tab while the menu is open.
        layout.activateTab('TimingWidget');
        await layout.components.get('TimingWidget').tabEl.updateComplete;
        await tick();
        await pane.updateComplete;

        const checked = menuItems(pane)
            .filter(item => item.getAttribute('aria-checked') === 'true')
            .map(item => item.querySelector('.ide-tab-menu-title').textContent);
        assert.deepEqual(checked, ['Timing']);
    });

    it('closes the overflow menu from the button that opened it', async () => {
        const { pane, btn } = await mountOverflowingPane({ open: true });

        // Pressing the ▾ must not take focus out of the menu: the focusout
        // would close it, and the click would then open it again.
        const press = new window.MouseEvent('mousedown', { bubbles: true, cancelable: true });
        btn.dispatchEvent(press);
        assert.ok(press.defaultPrevented);

        btn.click();
        await pane.updateComplete;
        assert.equal(menuOf(pane), null);
        assert.equal(pane.shadowRoot.activeElement, btn);
    });

    it('keeps focus in the overflow menu when the focused item goes away', async () => {
        const { pane, btn } = await mountOverflowingPane({ open: true });
        pressKey(pane.shadowRoot.activeElement, 'End');
        assert.equal(focusedTitle(pane), 'Charts');

        // Chrome blurs a focused node as it removes it; jsdom does not.
        const focused = pane.shadowRoot.activeElement;
        const remove = focused.remove.bind(focused);
        focused.remove = () => {
            focused.dispatchEvent(new window.FocusEvent('focusout', {
                bubbles: true, composed: true, relatedTarget: null,
            }));
            remove();
        };

        // E.g. a Tcl command closing the window while the menu is open.
        layout.closeComponent('ChartsWidget');
        await tick();
        await pane.updateComplete;
        assert.ok(menuOf(pane));
        assert.equal(focusedTitle(pane), 'Inspector', 'back on the active tab\'s item');

        // A close the user did not ask for still hands focus back to the ▾.
        layout.maximizePane('right');
        await pane.updateComplete;
        assert.equal(menuOf(pane), null);
        assert.equal(pane.shadowRoot.activeElement, btn);
    });

    it('lists a tab added while the overflow menu is open', async () => {
        const { pane } = await mountOverflowingPane({ open: true });

        const extra = document.createElement('ide-tab');
        extra.slot = 'tabs';
        extra.title = 'Extra';
        pane.appendChild(extra);
        await tick();
        await pane.updateComplete;

        const titles = menuItems(pane)
            .map(item => item.querySelector('.ide-tab-menu-title').textContent);
        assert.ok(titles.includes('Extra'), titles.join());
    });

    it('observes the strip and the tabs\' full width, not each tab', async () => {
        const observers = [];
        const RealResizeObserver = globalThis.ResizeObserver;
        globalThis.ResizeObserver = class {
            constructor(callback) {
                this.callback = callback;
                this.observed = new Set();
                observers.push(this);
            }
            observe(el) { this.observed.add(el); }
            disconnect() { this.observed.clear(); }
        };
        try {
            // A fresh layout, so its panes are built with the fake observer.
            document.body.innerHTML = '<div id="gl-container"></div>';
            containerEl = document.getElementById('gl-container');
            layout = new GridLayoutManager(containerEl);
            const { pane, tabsEl } = await mountRightPane();
            const track = pane.shadowRoot.querySelector('.ide-tabs-track');

            const ro = observers.find(o => o.observed.has(tabsEl));
            assert.ok(ro, 'the tab strip is observed');
            assert.deepEqual([...ro.observed], [tabsEl, track]);

            // A tab mounted later resizes the track; nothing new to observe.
            layout.registerComponentFactoryFunction('ClockWidget', () => {});
            layout.focusComponent('ClockWidget');
            await tick();
            assert.deepEqual([...ro.observed], [tabsEl, track]);

            // Without dispatching scroll: only the observer can notice this.
            Object.defineProperty(tabsEl, 'scrollWidth', { value: 700, configurable: true });
            Object.defineProperty(tabsEl, 'clientWidth', { value: 300, configurable: true });
            // A narrower strip brings the active tab back into view.
            const scrolls = [];
            layout.components.get('ClockWidget').tabEl.scrollIntoView = (opts) => scrolls.push(opts);
            ro.callback([]);
            await pane.updateComplete;
            assert.ok(!pane.shadowRoot.querySelector('.ide-btn-overflow').classList.contains('idle'));
            assert.deepEqual(scrolls, [{ inline: 'nearest', block: 'nearest' }]);
        } finally {
            globalThis.ResizeObserver = RealResizeObserver;
        }
    });

    it('scrolls an overflowing tab strip horizontally with a vertical wheel', async () => {
        const { pane, tabsEl } = await mountRightPane();
        const spin = (init) => {
            const e = new window.WheelEvent('wheel', { bubbles: true, cancelable: true, ...init });
            tabsEl.dispatchEvent(e);
            return e;
        };

        stubTabStrip(tabsEl, { scrollWidth: 300, clientWidth: 300 });
        await pane.updateComplete;
        assert.ok(!spin({ deltaY: 100 }).defaultPrevented);
        assert.equal(tabsEl.scrollLeft, 0);

        stubTabStrip(tabsEl, { scrollWidth: 700, clientWidth: 300 });
        await pane.updateComplete;
        assert.ok(spin({ deltaY: 100 }).defaultPrevented);
        assert.equal(tabsEl.scrollLeft, 100);

        spin({ deltaY: 3, deltaMode: 1 });
        assert.equal(tabsEl.scrollLeft, 148);

        // The strip scrolls sideways, so a page is its width (300).
        spin({ deltaY: 1, deltaMode: 2 });
        assert.equal(tabsEl.scrollLeft, 448);

        // Horizontal gestures are left to the browser.
        assert.ok(!spin({ deltaX: 50, deltaY: 10 }).defaultPrevented);
        assert.equal(tabsEl.scrollLeft, 448);

        // So are Ctrl+wheel and trackpad pinch, which zoom the page.
        assert.ok(!spin({ deltaY: 100, ctrlKey: true }).defaultPrevented);
        assert.equal(tabsEl.scrollLeft, 448);
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
        assert.equal(maxBtn.textContent.trim(), '⧉ Restore layout');
        assert.ok(maxBtn.classList.contains('ide-btn-restore'));

        pane.setMaximized(false);
        await pane.updateComplete;
        assert.ok(!pane.hasAttribute('maximized'));
        assert.ok(!pane.maximized);
        assert.equal(maxBtn.textContent, '⤢');
        assert.ok(!maxBtn.classList.contains('ide-btn-restore'));

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
        assert.ok(tab.shadowRoot.querySelector('.ide-tab-icon svg'));

        // Test updating to another TabIcon
        tab.icon = TabIcon.TCL_CONSOLE;
        await tab.updateComplete;
        assert.ok(tab.shadowRoot.querySelector('.ide-tab-icon svg'));

        // Tabs without an icon do not render an icon container
        tab.icon = null;
        await tab.updateComplete;
        assert.equal(tab.shadowRoot.querySelector('.ide-tab-icon'), null);

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
