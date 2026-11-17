// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Search & navigation dialogs (issue #10619 table 2.8):
//  - Find: name/glob search by object type (mirrors Qt FindObjectDialog).
//  - Go to position: jump the view to x,y in microns (mirrors Qt GotoDialog).
// Both reuse the .modal-overlay/.modal-dialog pattern from menu-bar.js.

import { dbuToWorld, worldToDbu } from './coordinates.js';
import { applySelectionFlags, beginSelection, buildModal, isCurrentSelection,
         zoomToBBox } from './ui-utils.js';

// The .search-nav-dialog rules size these two dialogs' rows; every caller here
// passes it so buildModal itself stays dialog-agnostic.
const kDialogClass = 'search-nav-dialog';

// ── Find ──────────────────────────────────────────────────────────────────

const FIND_TYPES = [
    { label: 'Instance', value: 'inst' },
    { label: 'Net', value: 'net' },
    { label: 'Port', value: 'port' },
];

export function showFindDialog(app) {
    if (!app.designScale) {
        return;
    }
    const typeOptions = FIND_TYPES
        .map((t) => `<option value="${t.value}">${t.label}</option>`)
        .join('');
    const { overlay, close, okBtn, showError } = buildModal('Find Object', `
        <div class="sn-row">
            <label>Name</label>
            <input type="text" class="fb-path-input sn-pattern"
                   placeholder="name or glob (*, ?, [])">
        </div>
        <div class="sn-row">
            <label>Type</label>
            <select class="sn-type">${typeOptions}</select>
        </div>
        <label class="sn-check">
            <input type="checkbox" class="sn-case"> Match case
        </label>`, 'Find', kDialogClass);

    const patternInput = overlay.querySelector('.sn-pattern');
    const typeSel = overlay.querySelector('.sn-type');
    const caseChk = overlay.querySelector('.sn-case');
    patternInput.focus();

    function doFind() {
        const pattern = patternInput.value.trim();
        if (!pattern) {
            showError('Enter a name or glob pattern.');
            return;
        }
        if (!app.websocketManager) {
            return;
        }
        showError('Searching…', true);
        // Find takes the selection only once it has matches, and not from a
        // selection that started while it was searching.
        const issued = app.selectionToken;
        app.websocketManager
            .request({
                type: 'find',
                obj_type: typeSel.value,
                pattern,
                match_case: caseChk.checked,
            })
            .then((resp) => {
                const count = resp && resp.count ? resp.count : 0;
                if (!count) {
                    showError('No objects found.');
                    return;
                }
                if (!isCurrentSelection(app, issued)) {
                    showError(`${count} found.`, true);
                    return;
                }
                beginSelection(app);
                // The outline is the last clicked object's; Find replaced it.
                app.clearSelectionOutline?.();
                applySelectionFlags(app, resp);
                // Auto-zoom to the matched objects — an improvement over the Qt
                // Find dialog, which only selects without centering.
                if (Array.isArray(resp.bbox)) {
                    zoomToBBox(app, resp.bbox, 0);
                }
                if (typeof app.updateInspector === 'function') {
                    app.updateInspector(resp);
                }
                // The selection is drawn on the overlay only; this also
                // refreshes the Select Highlight panel.
                app.refreshOverlay?.();
                const shown = resp.selection_count || count;
                showError(resp.truncated
                    ? `${count} found (showing first ${shown}).`
                    : `${count} found.`, true);
            })
            .catch((err) => showError('Find failed: ' + err));
    }

    okBtn.addEventListener('click', doFind);
    patternInput.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') { e.preventDefault(); doFind(); }
    });
    return { close };
}

// ── Go to position ──────────────────────────────────────────────────────────

export function showGotoDialog(app) {
    if (!app.designScale || !app.view) {
        return;
    }
    // Every field is read and written in whichever unit the "Show DBU"
    // setting (Qt's MainWindow::useDBU) is displaying, so the numbers here
    // match the ones the Inspector, the rulers and the scale bar are showing.
    const fmt = (dbu) => app.formatDbu(dbu);
    const parse = (str) => app.parseDbu(str);
    const unit = app.unitLabel();

    // Prefill X/Y with the current view center, so "Go to" with no edits is
    // a no-op.
    const c = app.view.getCenter();
    const centerDbu = worldToDbu(c[0], c[1], app.designScale,
        app.designOriginX, app.designOriginY);

    const { overlay, close, okBtn, showError } = buildModal('Go to Position', `
        <div class="sn-row"><label>X (${unit})</label>
            <input type="text" class="fb-path-input sn-x" value="${fmt(centerDbu.dbuX)}"></div>
        <div class="sn-row"><label>Y (${unit})</label>
            <input type="text" class="fb-path-input sn-y" value="${fmt(centerDbu.dbuY)}"></div>
        <div class="sn-row"><label>Size (${unit})</label>
            <input type="text" class="fb-path-input sn-size" placeholder="optional zoom window"></div>`,
        'Go to', kDialogClass);

    const xIn = overlay.querySelector('.sn-x');
    const yIn = overlay.querySelector('.sn-y');
    const sizeIn = overlay.querySelector('.sn-size');
    xIn.focus();
    xIn.select();

    function doGoto() {
        const dbuX = parse(xIn.value);
        const dbuY = parse(yIn.value);
        if (dbuX === null || dbuY === null) {
            showError(`X and Y must be numbers (${unit}).`);
            return;
        }

        // A size of 0 or less is not a window; fall through to the recentre.
        const sizeVal = sizeIn.value.trim();
        const sizeDbu = sizeVal ? parse(sizeVal) : null;
        if (sizeDbu !== null && sizeDbu > 0) {
            const half = sizeDbu / 2;
            zoomToBBox(app, [dbuX - half, dbuY - half, dbuX + half, dbuY + half],
                       0);
        } else {
            app.view.setView(dbuToWorld(dbuX, dbuY, app.designScale,
                                        app.designOriginX, app.designOriginY),
                             app.view.getZoom());
        }
        close();
    }

    okBtn.addEventListener('click', doGoto);
    for (const el of [xIn, yIn, sizeIn]) {
        el.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') { e.preventDefault(); doGoto(); }
        });
    }
    return { close };
}
