// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Creates a menu bar in #menu-bar and returns keyboard shortcut bindings.
export function createMenuBar(app) {
    const menus = [
        { label: 'File', items: [
            { label: 'Open DB...', action: () => showPathDialog(app, 'Open DB', 'read_db'),
              enabledWhen: () => !app.designScale },
            { label: 'Save DB...', action: () => showPathDialog(app, 'Save DB', 'write_db'),
              enabledWhen: () => !!app.designScale },
        ]},
        { label: 'View', items: [
            { label: 'Fit', shortcut: 'F', action: () => { if (app.fitBounds) app.map.fitBounds(app.fitBounds); } },
            { label: 'Zoom In', shortcut: 'Z', action: () => app.map.zoomIn() },
            { label: 'Zoom Out', shortcut: 'Shift+Z', action: () => app.map.zoomOut() },
            { type: 'separator' },
            { label: 'Toggle Theme', shortcut: 'T', action: () => app.toggleTheme() },
            { type: 'separator' },
            { label: 'Find...', shortcut: 'Ctrl+F', disabled: true },
            { label: 'Go to Position...', shortcut: 'Shift+G', disabled: true },
        ]},
        { label: 'Tools', items: [
            { label: 'Ruler', shortcut: 'K',
              action: () => { if (app.rulerManager) app.rulerManager.toggleRulerMode(); } },
            { label: 'Clear Rulers', shortcut: 'Shift+K',
              action: () => { if (app.rulerManager) app.rulerManager.clearAllRulers(); } },
        ]},
        { label: 'Windows', items: [
            { label: 'Layout Viewer', action: () => app.focusComponent('LayoutViewer') },
            { label: 'Display Controls', action: () => app.focusComponent('DisplayControls') },
            { label: 'Inspector', action: () => app.focusComponent('Inspector') },
            { label: 'Tcl Console', action: () => app.focusComponent('TclConsole') },
            { label: 'Hierarchy Browser', action: () => app.focusComponent('Browser') },
            { label: 'Timing', action: () => app.focusComponent('TimingWidget') },
            { label: 'DRC Viewer', action: () => app.focusComponent('DRCWidget') },
            { label: 'Clock Tree', action: () => app.focusComponent('ClockWidget') },
            { label: 'Charts', action: () => app.focusComponent('ChartsWidget') },
            { label: 'Help', action: () => app.focusComponent('HelpWidget') },
        ]},
        { label: 'Help', items: [
            { label: 'Keyboard Shortcuts', action: () => app.focusComponent('HelpWidget') },
        ]},
    ];

    const bar = document.getElementById('menu-bar');
    let openMenu = null;

    function closeAll() {
        if (openMenu) {
            openMenu.classList.remove('open');
            openMenu = null;
        }
    }

    for (const menu of menus) {
        const label = document.createElement('div');
        label.className = 'menu-label';
        label.textContent = menu.label;

        const dropdown = document.createElement('div');
        dropdown.className = 'menu-dropdown';

        for (const item of menu.items) {
            if (item.type === 'separator') {
                const sep = document.createElement('div');
                sep.className = 'menu-separator';
                dropdown.appendChild(sep);
                continue;
            }

            const row = document.createElement('div');
            row.className = 'menu-item';
            if (item.disabled) row.classList.add('disabled');
            // Track items with dynamic enabled state for refresh on open.
            if (item.enabledWhen) row._enabledWhen = item.enabledWhen;

            const text = document.createElement('span');
            text.textContent = item.label;
            row.appendChild(text);

            if (item.shortcut) {
                const shortcut = document.createElement('span');
                shortcut.className = 'shortcut';
                shortcut.textContent = item.shortcut;
                row.appendChild(shortcut);
            }

            if (item.action) {
                row.addEventListener('click', (e) => {
                    if (row.classList.contains('disabled')) return;
                    e.stopPropagation();
                    closeAll();
                    item.action();
                });
            }

            dropdown.appendChild(row);
        }

        label.appendChild(dropdown);

        function refreshDynamicItems() {
            for (const row of dropdown.querySelectorAll('.menu-item')) {
                if (row._enabledWhen) {
                    row.classList.toggle('disabled', !row._enabledWhen());
                }
            }
        }

        label.addEventListener('click', (e) => {
            e.stopPropagation();
            if (openMenu === label) {
                closeAll();
            } else {
                closeAll();
                refreshDynamicItems();
                label.classList.add('open');
                openMenu = label;
            }
        });

        // Hover-open when another menu is already open
        label.addEventListener('mouseenter', () => {
            if (openMenu && openMenu !== label) {
                closeAll();
                refreshDynamicItems();
                label.classList.add('open');
                openMenu = label;
            }
        });

        bar.appendChild(label);
    }

    document.addEventListener('click', closeAll);
}

// ─── File Browser Dialog (Open/Save DB) ─────────────────────────────────────

function showPathDialog(app, title, tclCmd) {
    const isSave = tclCmd === 'write_db';
    const overlay = document.createElement('div');
    overlay.className = 'modal-overlay';

    overlay.innerHTML = `
        <div class="modal-dialog file-browser-dialog">
            <h3>${title}</h3>
            <div class="fb-breadcrumb"></div>
            <div class="fb-file-list"></div>
            <input type="text" class="fb-path-input"
                   placeholder="Server-side file path (e.g. /path/to/design.odb)">
            <div class="modal-error" style="display:none"></div>
            <div class="modal-buttons">
                <button class="cancel">Cancel</button>
                <button class="primary ok" disabled>${isSave ? 'Save' : 'Open'}</button>
            </div>
        </div>`;

    document.body.appendChild(overlay);

    const breadcrumb = overlay.querySelector('.fb-breadcrumb');
    const fileList = overlay.querySelector('.fb-file-list');
    const pathInput = overlay.querySelector('.fb-path-input');
    const errorDiv = overlay.querySelector('.modal-error');
    const okBtn = overlay.querySelector('.ok');
    const cancelBtn = overlay.querySelector('.cancel');
    let currentPath = '';
    let selectedEntry = null;

    function close() {
        overlay.remove();
    }

    function updateOkState() {
        const val = pathInput.value.trim();
        okBtn.disabled = !val;
    }

    function renderBreadcrumb(dirPath) {
        breadcrumb.innerHTML = '';
        const parts = dirPath.split('/').filter(Boolean);
        // Root
        const rootSpan = document.createElement('span');
        rootSpan.className = 'fb-crumb';
        rootSpan.textContent = '/';
        rootSpan.addEventListener('click', () => navigate('/'));
        breadcrumb.appendChild(rootSpan);

        let accumulated = '';
        for (const part of parts) {
            accumulated += '/' + part;
            const sep = document.createElement('span');
            sep.className = 'fb-crumb-sep';
            sep.textContent = ' / ';
            breadcrumb.appendChild(sep);

            const crumb = document.createElement('span');
            crumb.className = 'fb-crumb';
            crumb.textContent = part;
            const target = accumulated;
            crumb.addEventListener('click', () => navigate(target));
            breadcrumb.appendChild(crumb);
        }
    }

    function renderEntries(entries) {
        fileList.innerHTML = '';
        selectedEntry = null;

        if (!entries || entries.length === 0) {
            const empty = document.createElement('div');
            empty.className = 'fb-empty';
            empty.textContent = '(empty directory)';
            fileList.appendChild(empty);
            return;
        }

        for (const entry of entries) {
            const row = document.createElement('div');
            row.className = 'fb-entry';
            if (entry.is_dir) row.classList.add('fb-dir');

            const icon = document.createElement('span');
            icon.className = 'fb-icon';
            icon.textContent = entry.is_dir ? '\u{1F4C1}' : '\u{1F4C4}';
            row.appendChild(icon);

            const name = document.createElement('span');
            name.className = 'fb-name';
            name.textContent = entry.name;
            row.appendChild(name);

            if (!entry.is_dir && entry.size !== undefined) {
                const size = document.createElement('span');
                size.className = 'fb-size';
                size.textContent = formatSize(entry.size);
                row.appendChild(size);
            }

            if (entry.is_dir) {
                row.addEventListener('click', () => {
                    navigate(currentPath + '/' + entry.name);
                });
            } else {
                row.addEventListener('click', () => {
                    // Deselect previous
                    const prev = fileList.querySelector('.fb-selected');
                    if (prev) prev.classList.remove('fb-selected');
                    row.classList.add('fb-selected');
                    selectedEntry = entry;
                    pathInput.value = currentPath + '/' + entry.name;
                    updateOkState();
                });
                row.addEventListener('dblclick', () => {
                    pathInput.value = currentPath + '/' + entry.name;
                    updateOkState();
                    submit();
                });
            }

            fileList.appendChild(row);
        }
    }

    async function navigate(dirPath) {
        errorDiv.style.display = 'none';
        fileList.innerHTML = '<div class="fb-loading">Loading...</div>';

        try {
            const resp = await app.websocketManager.request({
                type: 'list_dir',
                path: dirPath || '',
            });
            currentPath = resp.path;
            renderBreadcrumb(resp.path);
            renderEntries(resp.entries);
            // For save mode, keep the filename if user typed one
            if (isSave) {
                const existing = pathInput.value.trim();
                const basename = existing.split('/').pop();
                if (basename && !basename.includes('/')) {
                    pathInput.value = currentPath + '/' + basename;
                } else {
                    pathInput.value = currentPath + '/';
                }
            } else {
                pathInput.value = '';
                selectedEntry = null;
            }
            updateOkState();
        } catch (err) {
            fileList.innerHTML = '';
            errorDiv.textContent = err.message || String(err);
            errorDiv.style.display = '';
        }
    }

    async function submit() {
        const path = pathInput.value.trim();
        if (!path) return;

        okBtn.disabled = true;
        okBtn.textContent = 'Working...';
        errorDiv.style.display = 'none';

        try {
            const resp = await app.websocketManager.request({
                type: 'tcl_eval',
                cmd: `${tclCmd} ${path}`,
            });

            if (resp.error) {
                errorDiv.textContent = resp.output || resp.result || 'Command failed';
                errorDiv.style.display = '';
                okBtn.disabled = false;
                okBtn.textContent = isSave ? 'Save' : 'Open';
                return;
            }

            close();

            if (tclCmd === 'read_db') {
                // Reload the page so the full init flow runs against the
                // newly loaded design (bounds, tech, display controls, etc.).
                window.location.reload();
                return;
            }
        } catch (err) {
            errorDiv.textContent = err.message || 'Request failed';
            errorDiv.style.display = '';
            okBtn.disabled = false;
            okBtn.textContent = isSave ? 'Save' : 'Open';
        }
    }

    pathInput.addEventListener('input', updateOkState);
    pathInput.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') {
            const val = pathInput.value.trim();
            // If it looks like a directory path (ends with /), navigate there
            if (val.endsWith('/')) {
                navigate(val.replace(/\/+$/, '') || '/');
            } else {
                submit();
            }
        }
        if (e.key === 'Escape') close();
    });
    okBtn.addEventListener('click', submit);
    cancelBtn.addEventListener('click', close);
    overlay.addEventListener('click', (e) => { if (e.target === overlay) close(); });

    // Start browsing from server's cwd
    navigate('');
}

function formatSize(bytes) {
    if (bytes < 1024) return bytes + ' B';
    if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB';
    if (bytes < 1024 * 1024 * 1024) return (bytes / (1024 * 1024)).toFixed(1) + ' MB';
    return (bytes / (1024 * 1024 * 1024)).toFixed(1) + ' GB';
}
